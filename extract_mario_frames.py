import os
from PIL import Image
from agon_image_converter import AGON_PALETTE_RGB, find_closest_color_index

def extract_frames_auto(sheet_path, output_dir, n_rows=2, transparent_color=None):
    os.makedirs(output_dir, exist_ok=True)
    sheet = Image.open(sheet_path).convert('RGBA')
    sheet_w, sheet_h = sheet.size
    # Analizza la prima riga per trovare i frame
    y = 0
    row_height = None
    # Trova la prima riga non trasparente
    for yy in range(sheet_h):
        if any(sheet.getpixel((xx, yy))[3] != 0 for xx in range(sheet_w)):
            y = yy
            break
    # Trova l'altezza del frame (fino a che non torna trasparente)
    for yy in range(y, sheet_h):
        if all(sheet.getpixel((xx, yy))[3] == 0 or sheet.getpixel((xx, yy))[:3] == transparent_color for xx in range(sheet_w)):
            row_height = yy - y
            break
    if row_height is None:
        row_height = 16  # fallback
    # Trova i bordi dei frame nella prima riga
    x = 0
    frame_boxes = []
    while x < sheet_w:
        # Salta trasparente
        while x < sheet_w and (sheet.getpixel((x, y))[3] == 0 or sheet.getpixel((x, y))[:3] == transparent_color):
            x += 1
        if x >= sheet_w:
            break
        start_x = x
        # Trova fine frame
        while x < sheet_w and not (sheet.getpixel((x, y))[3] == 0 or sheet.getpixel((x, y))[:3] == transparent_color):
            x += 1
        end_x = x
        frame_boxes.append((start_x, y, end_x, y + row_height))
    # Estrai frame per le prime due righe
    for row in range(n_rows):
        y_offset = y + row * row_height
        for idx, (start_x, _, end_x, _) in enumerate(frame_boxes):
            frame = sheet.crop((start_x, y_offset, end_x, y_offset + row_height))
            # Set transparency
            if transparent_color is not None:
                datas = frame.getdata()
                newData = []
                for item in datas:
                    if item[:3] == transparent_color:
                        newData.append((item[0], item[1], item[2], 0))
                    else:
                        newData.append(item)
                frame.putdata(newData)
            frame.save(os.path.join(output_dir, f"mario_frame_{row}_{idx}.png"))


def convert_frame_to_agon_palette(frame_path, output_path, transparent_color=None):
    img = Image.open(frame_path).convert('RGBA')
    width, height = img.size
    pixels = list(img.getdata())
    agon_indices = []
    for px in pixels:
        if px[3] == 0 and transparent_color is not None:
            agon_indices.append(0)  # Index 0 for transparency
        else:
            agon_indices.append(find_closest_color_index(px[:3]))
    # Save as C header
    var_name = os.path.splitext(os.path.basename(frame_path))[0]
    c_code = f"#ifndef {var_name.upper()}_H\n"
    c_code += f"#define {var_name.upper()}_H\n\n"
    c_code += f"#include <stdint.h>\n\n"
    c_code += f"#define {var_name.upper()}_WIDTH  {width}\n"
    c_code += f"#define {var_name.upper()}_HEIGHT {height}\n\n"
    c_code += f"static const uint8_t {var_name}_data[] = {{\n    "
    for i, idx in enumerate(agon_indices):
        if i % 16 == 0:
            c_code += "\n    "
        c_code += f"0x{idx:02X}, "
    c_code = c_code.rstrip(", ") + "\n};\n\n#endif // " + var_name.upper() + "\n"
    with open(output_path, 'w') as f:
        f.write(c_code)
    print(f"Converted {frame_path} to {output_path}")

if __name__ == "__main__":
    # Parameters (customize as needed)
    SHEET = "mariobros-spritesheet.png"
    OUTDIR = "mario_frames"
    TRANSPARENT = (51, 51, 51)    # Grigio scuro come trasparente
    extract_frames_auto(SHEET, OUTDIR, n_rows=2, transparent_color=TRANSPARENT)
    for fname in os.listdir(OUTDIR):
        if fname.endswith('.png'):
            convert_frame_to_agon_palette(os.path.join(OUTDIR, fname), os.path.join(OUTDIR, fname.replace('.png', '.h')), transparent_color=TRANSPARENT)
