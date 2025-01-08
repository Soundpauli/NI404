import os
import json
import shutil
import wave
import audioop

# Configuration
json_file = "categorized_audio_files.json"  # JSON file path
source_folder = "sounds"  # Folder containing original files
output_folder = "categorized_audio_files"  # Output folder

def process_wav_file(file_path, new_path):
    """
    Processes a .wav file: converts to mono, 16-bit, and 44100 Hz.
    """
    with wave.open(file_path, 'rb') as wav_file:
        params = wav_file.getparams()
        frames = wav_file.readframes(params.nframes)

        # Convert to mono
        if params.nchannels > 1:
            frames = audioop.tomono(frames, params.sampwidth, 1, 0)

        # Convert sample width to 16-bit
        if params.sampwidth != 2:
            frames = audioop.lin2lin(frames, params.sampwidth, 2)

        # Resample to 44100 Hz
        if params.framerate != 44100:
            frames, _ = audioop.ratecv(frames, 2, 1, params.framerate, 44100, None)

        # Write processed data to the new file
        with wave.open(new_path, 'wb') as new_wav_file:
            new_wav_file.setnchannels(1)
            new_wav_file.setsampwidth(2)
            new_wav_file.setframerate(44100)
            new_wav_file.writeframes(frames)

def create_folders(base_folder):
    """
    Creates numbered folders (0-9) inside the base folder.
    """
    for i in range(10):
        folder_path = os.path.join(base_folder, str(i))
        os.makedirs(folder_path, exist_ok=True)

def copy_and_process_files(json_file, source_folder, base_folder):
    """
    Copies files into categorized folders, processes and renames them.
    """
    with open(json_file, 'r') as f:
        categories = json.load(f)

    for i, (category, files) in enumerate(categories.items()):
        folder_index = i % 10  # Map category to folder (0-9)
        folder_path = os.path.join(base_folder, str(folder_index))
        
        count = 0  # Counter for renaming within each folder
        for file in files:
            try:
                # Construct source and destination paths
                source_path = os.path.join(source_folder, file)
                new_file_name = f"_{folder_index}{count:02}.wav"  # e.g., _300.wav
                destination_path = os.path.join(folder_path, new_file_name)

                # Process and copy the file
                if os.path.exists(source_path):
                    process_wav_file(source_path, destination_path)
                    count += 1
                else:
                    print(f"Source file not found: {source_path}")
            except Exception as e:
                print(f"Error processing {file}: {e}")

if __name__ == "__main__":
    create_folders(output_folder)
    copy_and_process_files(json_file, source_folder, output_folder)
    print