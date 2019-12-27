# NuMediaSDK
NuMediaSDK is a media software development kit for Nuvoton's ARM9 MPUs. It helps to simplify programming of media player and recorder.

----
## Major components in this repository
- BSP/ -- Nuvoton ARM9 family BSP
- Doc/ -- Documents
- Package/ -- Libraries source
- PrebuildLib/ -- Prebuild libraries
- SampleCode/ -- Sample programs
- TestCode/ -- Test programs for ThirdParty and Package libraries
- ThirdParty/ -- Third party libraries

----
## Supported Media and Codec
Media            |Video Codec      |Audio Codec
:----------------|-----------------|---------------
AVI              |H.264(baseline)  |AAC-LC, G711
MP4              |H.264(baseline)  |AAC-LC, G711

----
## How to run
1. git clone --recursive https://github.com/OpenNuvoton/NuMediaSDK.git
2. Browsing into SampleCode folder and double click uVersion project file (KEIL/xxx.uvproj)
3. Enter Keil compile mode  
a. Build  
b. Download  
4. Enter debug mode  
a. Run

----
## TODO list
1. Recorder
2. MP3 codec