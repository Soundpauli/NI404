import os
import json
import shutil

# Configuration
json_file = "categorized_audio_files.json"  # Replace with your JSON file path
source_folder = "sounds"  # Folder containing the original files
output_folder = "sd-folder"  # Output base folder

# Create folders (0-9)
def create_folders(base_folder):
    for i in range(10):
        folder_path = os.path.join(base_folder, str(i))
        os.makedirs(folder_path, exist_ok=True)

# Copy files into corresponding folders
def copy_files(json_file, source_folder, base_folder):
    with open(json_file, 'r') as f:
        categories = json.load(f)

    for i, (category, files) in enumerate(categories.items()):
        folder_index = i % 10  # Map category index to folder (0-9)
        folder_path = os.path.join(base_folder, str(folder_index))
        
        for file in files:
            try:
                # Construct source and destination paths
                source_path = os.path.join(source_folder, file)
                destination_path = os.path.join(folder_path, os.path.basename(file))

                # Check if the source file exists
                if os.path.exists(source_path):
                    shutil.copy(source_path, destination_path)
                else:
                    print(f"Source file not found: {source_path}")
            except Exception as e:
                print(f"Error copying {file} to {folder_path}: {e}")

if __name__ == "__main__":
    create_folders(output_folder)
    copy_files(json_file, source_folder, output_folder)
    print("Files categorized and copied successfully!")
