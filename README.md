# Mosquito
Hearing test exhibit developed for the SLO Children's Museum

This project uses an Arduino MEGA 2560 to control an Adafruit Wave shield, 11 arcade buttons, and 6 LED modules. 

Functionality: 2 users can put on a set of headphones and press the Start button when they are ready to begin the hearing test. The test will run through 5 different sounds that range in frequency. An LED module will indicate which sound the users are currently listening to. Since human hearing deteriorates as we age, different users will be able to hear only certain sounds. If the user heard the sound they will press a button that will light up to indicate that they heard the sound. Once the test is over the users can compare their results and determine who has the better hearing.

The Arduino code utilizes the WaveHC library for the wave shield and the PinChangeInt library for button input.
