# b *0x001000e4
# print *0xffffffbfc00000
# info registers r2

set architecture riscv:rv32
set disassemble-next-line on
set riscv use_compressed_breakpoint off

file hello_world
target remote :1234
