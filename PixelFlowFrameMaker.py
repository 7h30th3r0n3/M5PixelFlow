import os
import zipfile
from PIL import Image, ImageSequence
import shutil

def process_gif(gif_path, output_folder, start_index):
    """
    Process a single GIF file: extract frames, ensure at least 30 frames by looping,
    resize frames to 128x128, and copy to the final output folder with sequential renaming.

    :param gif_path: Path to the GIF file.
    :param output_folder: Folder to save the processed frames.
    :param start_index: Starting index for frame naming.
    :return: New starting index after processing the GIF.
    """
    temp_folder = os.path.join(output_folder, "temp")
    os.makedirs(temp_folder, exist_ok=True)

    # Extract frames from the GIF
    with Image.open(gif_path) as gif:
        frame_paths = []
        for i, frame in enumerate(ImageSequence.Iterator(gif)):
            frame = frame.convert('RGB')
            frame_resized = frame.resize((128, 128))
            frame_path = os.path.join(temp_folder, f"frame_{i:05d}.jpg")
            frame_resized.save(frame_path, "JPEG")
            frame_paths.append(frame_path)

    # Ensure at least 30 frames by looping
    if len(frame_paths) < 30:
        frame_paths = (frame_paths * (30 // len(frame_paths) + 1))[:30]

    # Copy frames to the output folder with renaming
    for i, frame_path in enumerate(frame_paths, start=start_index):
        new_name = f"frame_{i:05d}.jpg"
        shutil.copy2(frame_path, os.path.join(output_folder, new_name))

    # Cleanup temporary folder
    shutil.rmtree(temp_folder)

    return start_index + len(frame_paths)

def process_gifs_in_folder(input_folder, output_folder):
    """
    Process all GIFs in the input folder, concatenate their frames, and save them to the output folder.

    :param input_folder: Folder containing GIF files.
    :param output_folder: Folder to save the processed frames.
    """
    os.makedirs(output_folder, exist_ok=True)

    # Find all GIF files in the input folder
    gif_files = [os.path.join(input_folder, f) for f in os.listdir(input_folder) if f.endswith('.gif')]

    current_index = 0
    for gif_path in gif_files:
        current_index = process_gif(gif_path, output_folder, current_index)

    # Compress the final output folder into a ZIP archive
    print(f"All frames processed and saved to {output_folder}")

if __name__ == "__main__":
    # Example usage
    input_gif_folder = "./"
    output_frames_folder = "./output_frames"

    process_gifs_in_folder(input_gif_folder, output_frames_folder)
