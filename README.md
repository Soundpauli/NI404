
DIY OPEN SOURCE HARDWARE SAMPLER-SEQUENCER
---------
An Etch-A-SketchTM inspired device for beginners and makers.


THE OPEN SOURCE SAMPLE-SEQUENCER FOR EVERYONE.

The NI404 is an innovative, open-source DIY sample-sequencer from soundpauli - inspired by the classic Etch-A-Sketch, tailored for beginners and DIY enthusiasts. Designed around the Teensy Microcontroller and Audioboard from PJRC, this device offers an accessible entry point into the world of music creation.
The NI404 is more than just a musical instrument; it's a journey into the world of sound exploration, perfect for live performances, studio sessions, or simply having fun creating music. Whether you're a seasoned musician or a curious beginner, the NI404 is your gateway to an exciting world of musical possibilities.

EASY TO USE.

With its playful design, the NI404 lets users create complex sound patterns without any prior knowledge of musical instruments or equipment. It's perfect for beginners and makers alike.

DYNAMIC CONTROL DESPITE LESS BUTTONS.

At the heart of the NI404 is a vivid 16x16 RGB LED panel display, paired with three intuitive rotary-push encoders. Navigate a cursor across the grid, where each row signifies a voice and each column a potential note, allowing for up to 16 pitches per note. This feature enables the mixing of up to 8 voices simultaneously.

REAL-TIME INTERACTION.

Adjust samples, instruments, pitches, notes, volume, BPM, velocity, and effects in real-time without pausing. This seamless interaction makes the NI404 an ideal gadget for live performances.

CORE COMPONENTS.

Teensy 4.1 Microcontroller and Teensy Audio board (4.0)
8MB PSRAM Flash Memory
3 Rotary-push encoders
16x16 RGB-LED Panel driven by FastLED
3.5mm audio jack and removable SD Microcard slot

CREATIVE FLEXIBILITY.

Etch-A-Sketch style note drawing
On-the-fly note deletion and sample muting
Adjustable volume, BPM (40 - 200), pattern, and note velocity
Up to 30-second sample length with seamless looping
Use your own samples on the SD-Card - in a simple file structure
Load up to 8 voices / samples (Wav format) plus an additional onboard synth voice
16-bar patterns across 8 pages, totaling 128 bars per song
Save and load up to 100 patterns / songs on the SD card
Autosave and autoload functions
SAMPLES AT YOUR FINGERTIPS:
THE BUILT-IN SAMPLE BROWSER.
Access and manage samples while playing, with support for up to 999 samples on the SD card and the ability to predefine multiple voices as a SampleSet.

FULLY INDEPENDENT.

Operates without a computer, generating all sounds on the device itself. USB powered (5V) and Arduino code is published under the MIT License.

Important: The NI404 does not include speakers or Bluetooth connectivity; headphones are recommended. Because of licensing: please use your own sample files (folder structure is included).

READY FOR MORE.

Potential updates include USB-MIDI In & Out, AI-powered song / pattern generation, WiFi connectivity, and a new aluminium casing.

HARDWARE LIST

Custom PCB - to connect the encoders to the Teensy (but you can also hard wire everything): see Downloads
Teensy 4.1
Teensy Audio Adaptor / TEENSY4_AUDIO
PSRAM Chip for Teensy 4.1
Rotary Encoder KY-040 360degree + Push
Jumper Wire Cables (10cm)
Micro SD Card (Class 10)
Micro SD-Extension
16x16 RGB LED Matrix
USED LIBRARIES
WS2812Serial
Teensy Audio Library (Audio.h, v1.0.3)
Quadrature Encoder Library (Encoder.h, v1.4.4)
Mapf (v1.0.2, GPL-3.0 license)
FastLED (v3.6.0, MIT License)
TeensyPolyphony (v1.0.7, MIT License)


LICENSE

The code is released under the MIT License, which means it's freely available for personal and commercial use, modification, distribution, and private copying. This permissive license is part of our effort to support innovation and creativity.

However, while the NI404 code itself is under the MIT License, we urge every user to be mindful of the dependencies and libraries utilized within the code. Each of these components may be governed by its own set of licenses. It's crucial for users to verify and ensure that they are in compliance with the licenses of all dependencies and libraries used in their projects. This step is important to respect the legal and ethical frameworks of open-source software.

So, whether you're a seasoned developer or a curious beginner, feel free to dive into the code, explore, modify, and create. Just remember to stay informed and respectful of the licensing requirements of all software components you use.


BUY A NI404

If you don't like to build your own NI404 (which i would recommend, it's an awesome project even for beginners), you can purchase one. Please hit me up at JPKuntoff@gmail.com and let's talk.

GRATITUDE

I would like to extend my heartfelt thanks to Paul Stoffregen and everyone involved at PJRC for their incredible work in creating the Teensy environment and hardware. The innovation and dedication evident in Teensy have significantly impacted the DIY and maker communities, enabling countless creative projects and technological advancements.

Additionally, my gratitude goes out to the broader community of developers and enthusiasts who have contributed libraries for this remarkable piece of hardware. Your collective efforts and shared knowledge have not only enhanced the capabilities of Teensy but have also fostered a spirit of collaboration and open-source development, especially to Nic Newdigate, who contributed the awesome teensy-polyphony library, which is the "soul" of this project. Thanks!

Thank you all for your hard work, ingenuity, and commitment to making the Teensy platform a powerful and accessible tool for creators around the world.

Jan from soundpauli
Hamburg, January 2024
