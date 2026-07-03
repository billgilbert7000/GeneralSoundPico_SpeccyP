#!/usr/bin/env python3
import sys
import os

def bin_to_c(input_file, output_file):
    with open(input_file, 'rb') as f:
        data = f.read()
    
    with open(output_file, 'w') as f:
        f.write('// Автоматически сгенерированный файл из gm_bank.bin\n')
        f.write('#include <stdint.h>\n\n')
        f.write('const uint8_t gm_bank_data[] = {\n')
        
        # Пишем по 16 байт в строке
        for i in range(0, len(data), 16):
            f.write('    ')
            chunk = data[i:i+16]
            hex_bytes = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(hex_bytes)
            if i + 16 < len(data):
                f.write(',\n')
            else:
                f.write('\n')
        
        f.write('};\n\n')
        f.write(f'const uint32_t gm_bank_data_size = {len(data)};\n')

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python3 bin_to_c.py <input.bin> <output.c>')
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f'Error: Input file {input_file} not found!')
        sys.exit(1)
    
    bin_to_c(input_file, output_file)
    print(f'Converted {input_file} to {output_file}')