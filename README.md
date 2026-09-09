# zifon-5000-camera-zoom
Camera zoom control using the original Zifon 5000 controller, Arduino and NRF24L01.
Project Overview

The Zifon 5000 motorized camera head is primarily designed for still cameras, and its original wireless controller does not provide direct control of a video camera's zoom.

The goal of this project is to extend the functionality of the original controller so that it can be used not only to control the movement of the motorized head, but also to operate the zoom of a video camera.

An Arduino with an NRF24L01 module is used to intercept the original wireless communication transmitted by the Zifon controller. The Arduino receives and analyzes the transmitted packets, detects the relevant commands, and converts them into signals suitable for the camera's remote zoom interface.

The H and V buttons on the original controller are used for zoom control:

H – Zoom In
V – Zoom Out
<img width="963" height="792" alt="20260907_170853" src="https://github.com/user-attachments/assets/3ec49e07-c579-43cc-82c7-b7a2ac55a5a3" />

The useful feature of these buttons is that pressing either button on its own generates a wireless command, while the motorized head itself does not react to it. This makes it possible to use the H and V buttons for zoom control without interfering with the original operation of the Zifon head.

Inspiration and Zifon 5000 Communication

This project was inspired by the work of jdesbonnet and the zifon_pt5000 project:

https://github.com/jdesbonnet/zifon_pt5000

Many thanks to the author for publishing the results of the Zifon 5000 wireless communication analysis. This information was extremely helpful during the development of this project.

During further investigation of the communication protocol, I also identified how the radio address changes when switching between controller channels.

For each subsequent channel, the value of every byte in the radio address is incremented by 2.

For example:

Channel 1
0xA3, 0xAA, 0x16, 0x0C, 0x02
Channel 2
0xA5, 0xAC, 0x18, 0x0E, 0x04
Channel 3
0xA7, 0xAE, 0x1A, 0x10, 0x06

This pattern makes it possible to derive the radio addresses used by the other controller channels.

How It Works

The basic signal flow is:

Zifon 5000 Controller
          │
          │  2.4 GHz
          ▼
       NRF24L01
          │
          ▼
        Arduino
          │
          ▼
 Camera Zoom Interface
          │
          ▼
      Video Camera

The NRF24L01 receives packets transmitted by the original Zifon 5000 wireless controller.

The Arduino analyzes the received data and detects commands corresponding to the H and V buttons. These commands are then converted into the signals required by the video camera's remote zoom interface.

The original Zifon controller can therefore be used simultaneously for:

controlling the movement of the motorized camera head,
Zoom In,
Zoom Out.

There is no need for a separate handheld zoom controller.

Hardware

The project uses:

Original Zifon 5000 wireless controller
Arduino
NRF24L01 2.4 GHz transceiver module
Video camera with external zoom control capability
Camera interface electronics
Power supply
Connecting cables
Schematic

The schematic of the Arduino, NRF24L01, and camera interface will be included here.

<!-- Add schematic image here, for example: ![Schematic](images/schematic.png) -->
<img width="527" height="401" alt="schema_obr" src="https://github.com/user-attachments/assets/bce03832-f12f-4d70-b77d-5d5329b447fa" />

Implementation

The completed hardware implementation and connection to the camera will be shown here.

<!-- Add implementation photos here, for example: ![Hardware implementation](images/hardware.jpg) -->
<img width="966" height="459" alt="20260907_170515" src="https://github.com/user-attachments/assets/6bbcd41f-b911-4476-b3a9-64d9224ecf09" />

Features
Original Zifon 5000 controller remains fully usable
Wireless reception of the original Zifon commands using NRF24L01
Arduino-based packet decoding
Zoom In / Zoom Out control
H and V buttons reused without modifying the original controller
No separate handheld zoom controller required
No modification of the Zifon transmitter required
Possibility of adding additional camera functions in the future
Project Status

The project is currently under development.

Additional technical documentation will be added, including:

NRF24L01 configuration
Captured packet structure
Zifon 5000 command mapping
Arduino source code
Camera interface schematic
Complete wiring diagram
Photos of the completed hardware
Test results
Acknowledgements

Special thanks to jdesbonnet for publishing the analysis of the Zifon 5000 wireless communication:

https://github.com/jdesbonnet/zifon_pt5000

The information provided by this project was an important starting point for my own experiments and further analysis.
<img width="920" height="923" alt="20260907_170829" src="https://github.com/user-attachments/assets/c5c4b058-20e3-4130-aa75-4b8ff9e552ab" />

Disclaimer

This is an independent experimental DIY project and is not affiliated with, endorsed by, or supported by Zifon.

The project involves reverse engineering and modification of electronic hardware. All information is provided for experimental and educational purposes. Use it at your own risk.
