# Ball Drop

An Arduino Uno game on a 8 x 8 LED Matrix
![Image of the project](https://i.imgur.com/Acfqlw9.jpg)

## Components required

For this project you will need:

- Arduino UNO

- 2 x Medium Breadboard

- Red 8x8 LED matrix

- 1 x Max7219 driver

- LCD 1602 Module

- 10 μF, 0.1 μF capacitors

- 10k, 1k, 220 Ohm resistors

- Joystick Arduino Module

- Jumper wires male-male (lots of them)

- 5 x Jumper wires male-female (for the joystick)

## About the game

In this game you are a ball that has to try to get as far as possible without being squashed by the platforms on the top of the screen.

### Scoring system

There's a highscore that gets saved on the arduino and you can always try to beat the previous highscore.
The score is calculated by "how long you survive", every 300 ms you get some points (calculated by the current speed of the platforms).

### Difficulty

The difficulty consists of constantly raising the speed of the platforms, they start from Speed 1 all the way to Speed 10

## Video

Click on this [link](https://www.youtube.com/embed/WU3zjfFL5Lk) to see the whole project in action.
