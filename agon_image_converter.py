import argparse
from PIL import Image
import os

# AGON palette (from agon_palette.h)
AGON_PALETTE_RGB = [
    (0, 0, 0), (0, 0, 85), (0, 0, 170), (0, 0, 255),
    (0, 85, 0), (0, 85, 85), (0, 85, 170), (0, 85, 255),
    (0, 170, 0), (0, 170, 85), (0, 170, 170), (0, 170, 255),
    (0, 255, 0), (0, 255, 85), (0, 255, 170), (0, 255, 255),
    (85, 0, 0), (85, 0, 85), (85, 0, 170), (85, 0, 255),
    (85, 85, 0), (85, 85, 85), (85, 85, 170), (85, 85, 255),
    (85, 170, 0), (85, 170, 85), (85, 170, 170), (85, 170, 255),
    (85, 255, 0), (85, 255, 85), (85, 255, 170), (85, 255, 255),
    (170, 0, 0), (170, 0, 85), (170, 0, 170), (170, 0, 255),
    (170, 85, 0), (170, 85, 85), (170, 85, 170), (170, 85, 255),
    (170, 170, 0), (170, 170, 85), (170, 170, 170), (170, 170, 255),
    (170, 255, 0), (170, 255, 85), (170, 255, 170), (170, 255, 255),
    (255, 0, 0), (255, 0, 85), (255, 0, 170), (255, 0, 255),
    (255, 85, 0), (255, 85, 85), (255, 85, 170), (255, 85, 255),
    (255, 170, 0), (255, 170, 85), (255, 170, 170), (255, 170, 255),
    (255, 255, 0), (255, 255, 85), (255, 255, 170), (255, 255, 255)
]

def find_closest_color_index(rgb):
    """Finds the closest color in the AGON palette and returns its index."""
    min_dist = float('inf')
    best_index = -1
    for i, palette_color in enumerate(AGON_PALETTE_RGB):
        dist = sum([(c1 - c2) ** 2 for c1, c2 in zip(rgb, palette_color)])
        if dist < min_dist:
            min_dist = dist
            best_index = i
    return best_index

def convert_image_to_c_header(image_path, output_path, var_name):
    """Converts an image to a C header file with AGON palette indices."""
    try:
        img = Image.open(image_path).convert('RGB')
    except FileNotFoundError:
        print(f"Error: Image file not found at {image_path}")
        return

    width, height = img.size
    pixels = list(img.getdata())

    c_code = f"#ifndef {var_name.upper()}_H\n"
    c_code += f"#define {var_name.upper()}_H\n\n"
    c_code += f"#include <stdint.h>\n\n"
    c_code += f"#define {var_name.upper()}_WIDTH  {width}\n"
    c_code += f"#define {var_name.upper()}_HEIGHT {height}\n\n"
    c_code += f"static const uint8_t {var_name}_data[] = {{\n    "

    for i, pixel in enumerate(pixels):
        if i % 16 == 0:
            c_code += "\n    "
        index = find_closest_color_index(pixel)
        c_code += f"0x{index:02X}, "

    c_code = c_code.rstrip(", ") + "\n};\n\n#endif // " + var_name.upper() + "\n"

    with open(output_path, 'w') as f:
        f.write(c_code)
    print(f"Successfully converted {image_path} to {output_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert image to AGON VDP C header.')
    parser.add_argument('input', help='Input image file (e.g., sprite.png)')
    parser.add_argument('-o', '--output', help='Output C header file (e.g., sprite.h)')
    parser.add_argument('-n', '--name', help='Variable name for the data array (default: image_data)')
    args = parser.parse_args()

    output_file = args.output
    if not output_file:
        base, _ = os.path.splitext(args.input)
        output_file = base + '.h'

    var_name = args.name
    if not var_name:
        base, _ = os.path.splitext(os.path.basename(args.input))
        var_name = base + "_data"
        
    convert_image_to_c_header(args.input, output_file, var_name)
