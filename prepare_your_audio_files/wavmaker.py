import os
import wave
import audioop

def process_wav_file(file_path, new_path):
    with wave.open(file_path, 'rb') as wav_file:
        params = wav_file.getparams()
        frames = wav_file.readframes(params.nframes)

        # Convert to Mono if needed
        if params.nchannels > 1:
            frames = audioop.tomono(frames, params.sampwidth, 1, 0)

        # Change sample width to 16-bit if needed
        if params.sampwidth != 2:
            frames = audioop.lin2lin(frames, params.sampwidth, 2)

        # Change sample rate to 44100 Hz if needed
        if params.framerate != 44100:
            frames, _ = audioop.ratecv(frames, 2, 1, params.framerate, 44100, None)

        # Write the processed data to a new file with the correct parameters
        with wave.open(new_path, 'wb') as new_wav_file:
            new_wav_file.setnchannels(1)
            new_wav_file.setsampwidth(2)
            new_wav_file.setframerate(44100)
            new_wav_file.writeframes(frames)

def format_wav_files(directory, start_number):
    count = start_number
    for filename in os.listdir(directory):
        if filename.endswith(".wav"):
            file_path = os.path.join(directory, filename)
            new_filename = f"_{count}.wav"
            new_file_path = os.path.join(directory, new_filename)

            process_wav_file(file_path, new_file_path)
            os.remove(file_path)  # Delete the original file after processing
            count += 1

# Get the directory of the Python script
directory = os.path.dirname(os.path.abspath(__file__))
start_number = int(input("Enter the starting number for renaming: "))

# Run the function
format_wav_files(directory, start_number)
