from PIL import Image, ImageDraw, ImageFont
import os

# Create a 170x320 dummy image
width, height = 170, 320

# Create a new image with a gradient background
img = Image.new('RGB', (width, height), color='lightblue')
draw = ImageDraw.Draw(img)

# Add a gradient effect
for y in range(height):
    color_intensity = int(255 * (1 - y / height))
    color = (100 + color_intensity//3, 150 + color_intensity//4, 200 + color_intensity//5)
    draw.line([(0, y), (width, y)], fill=color)

# Add some geometric shapes for visual interest
draw.rectangle([10, 10, width-10, 60], outline='darkblue', width=3)
draw.ellipse([20, 80, width-20, 160], fill='white', outline='darkblue', width=2)
draw.rectangle([30, 180, width-30, 220], fill='lightgreen', outline='darkgreen', width=2)

# Add text
try:
    # Try to use a default font, fallback to basic if not available
    font = ImageFont.load_default()
except:
    font = None

text_lines = [
    "170 x 320",
    "Dummy Image",
    "For nRF52",
    "Image Uploader",
    "Test"
]

y_pos = 240
for line in text_lines:
    text_bbox = draw.textbbox((0, 0), line, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    x_pos = (width - text_width) // 2
    draw.text((x_pos, y_pos), line, fill='darkblue', font=font)
    y_pos += 15

# Save the image
output_path = '/home/dani/projects/tq/hpr-disp-gui-wizzard-app/dummy_image_170x320.png'
img.save(output_path)
print(f"Dummy image created: {output_path}")
print(f"Image size: {img.size}")