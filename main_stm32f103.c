/*
 * Based on https://github.com/libopencm3/libopencm3-examples
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * Based on https://github.com/ve3wwg/stm32f103c8t6
 *
 * Copyright (C) 2017-2018 Warren Gay <ve3wwg@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#include <config.h>

#include <linux/compiler.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

static void usart_setup(void)
{
	//////////////////////////////////////////////////////////////
	// STM32F103C8T6:
	//	RX:	A9
	//	TX:	A10
	//////////////////////////////////////////////////////////////

	// GPIO_USART1_TX/GPIO13 on GPIO port A for tx
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	usart_enable(USART1);
}

static volatile char initialized = 0;			// True when USB configured
static QueueHandle_t usb_txq;				// USB transmit queue
static QueueHandle_t usb_rxq;				// USB receive queue

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483, /* 0483  STMicroelectronics, see http://www.linux-usb.org/usb-ids.html */
	.idProduct = 0x5740, /* 5740  Virtual COM Port */
	.bcdDevice = 0x0200, /* 2.00 */
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver. (Gareth McMullin <gareth@blacksphere.co.nz>)
 */
static const struct usb_endpoint_descriptor comm_endp[] = {
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x83,
		.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
		.wMaxPacketSize = 16,
		.bInterval = 255,
	}
};

static const struct usb_endpoint_descriptor data_endp[] = {
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = 64,
		.bInterval = 1,
	}, {
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x82,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = 64,
		.bInterval = 1,
	}
};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 }
};

static const struct usb_interface_descriptor comm_iface[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = USB_CLASS_CDC,
		.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
		.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
		.iInterface = 0,

		.endpoint = comm_endp,

		.extra = &cdcacm_functional_descriptors,
		.extralen = sizeof(cdcacm_functional_descriptors)
	}
};

static const struct usb_interface_descriptor data_iface[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,

		.endpoint = data_endp,
	}
};

static const struct usb_interface ifaces[] = {
	{
		.num_altsetting = 1,
		.altsetting = comm_iface,
	}, {
		.num_altsetting = 1,
		.altsetting = data_iface,
	}
};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char * usb_strings[] = {
	"libusbcdc.c driver", /* iManufacturer */
	"CDC-ACM module", /* iProduct */
	"WGDEMO", /* iSerial */
};

static enum usbd_request_return_codes
cdcacm_control_request(
  usbd_device *usbd_dev,
  struct usb_setup_data *req,
  uint8_t **buf,
  uint16_t *len,
  void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)
) {
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding)) {
			return USBD_REQ_NOTSUPP;
		}

		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	unsigned rx_avail = uxQueueSpacesAvailable(usb_rxq);	/* How much queue capacity left? */
	char buf[64];						/* rx buffer */
	int len;

	(void)ep;

	len = sizeof buf < rx_avail ? sizeof buf : rx_avail;	/* Bytes to read */
	len = usbd_ep_read_packet(usbd_dev, 0x01, buf, len);	/* Read what we can, leave the rest */

	if (len) {
		usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
		buf[len] = 0;
	}
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		cdcacm_control_request);

	initialized = 1;
}

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

static void usb_task(void *arg)
{
	usbd_device *udev = (usbd_device *)arg;
	char txbuf[64];
	unsigned txlen = 0;

	for (;;) {
		usbd_poll(udev);			/* Allow driver to do it's thing */
		if (initialized) {
			while (txlen < sizeof txbuf
				&& xQueueReceive(usb_txq, &txbuf[txlen], 0) == pdPASS)
				++txlen;		/* Read data to be sent */

			if (txlen > 0) {
				if (usbd_ep_write_packet(udev, 0x82, txbuf, txlen) != 0)
					txlen = 0;	/* Reset if data sent ok */
			} else	{
				taskYIELD();		/* Then give up CPU */
			}
		} else	{
			taskYIELD();
		}
	}
}

static void blink_led_task(void *args __attribute__((unused)))
{
	for (;;) {
		gpio_toggle(GPIOB, GPIO1);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void bus_spider_task(void *args __attribute__((unused)))
{
	extern void bus_spider_main(void);

	for (;;) {
		malloc_stats();

		bus_spider_main();
	}
}

static uint8_t malloc_base[4096] __aligned(16);

int main(void)
{
	usbd_device *udev = NULL;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	mem_malloc_init((void *)malloc_base,
			(void *)malloc_base + sizeof(malloc_base) - 1);

	// Clock for GPIO port A: GPIO_USART1_TX, USART1
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);

	// Clock for GPIO port B: LED (PB1)
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_USB);

	usart_setup();

	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);

	gpio_clear(GPIOB, GPIO1);
	xTaskCreate(blink_led_task, "blink_led", 350, NULL, 1, NULL);

	xTaskCreate(bus_spider_task, "bus_spider", 512, NULL, 1, NULL);

	usb_txq = xQueueCreate(128, sizeof(char));
	usb_rxq = xQueueCreate(128, sizeof(char));

	udev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config,
		usb_strings, 3,
		usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(udev, cdcacm_set_config);

	xTaskCreate(usb_task, "usb", 300, udev, 1, NULL);

	/* enable USB */
	gpio_clear(GPIOB, GPIO9);

	vTaskStartScheduler();

	for (;;)
		;

	return 0;
}
