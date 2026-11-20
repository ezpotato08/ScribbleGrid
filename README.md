# ScribbleGrid
# ScribbleGrid

**An Accessible XY Drawing Plotter**

ScribbleGrid is a tabletop drawing machine designed to make creative expression more accessible to individuals with limited upper-body mobility.

## Features

- Joystick-controlled XY positioning
- Trackball-based precision drawing
- Servo-controlled pen lifting
- Automatic homing using limit switches
- Software travel limits
- Coordinated XY motion using Bresenham's line algorithm

## Hardware

- Arduino Uno and CNC Shield V3
- 2 × NEMA 17 stepper motors
- 2 × DRV8825 motor drivers
- SG90 servo motor
- Analog joystick and PS/2 trackball
- GT2 belt-driven XY gantry

## Software

The Arduino firmware processes user inputs, performs homing, controls the stepper motor drivers, and coordinates drawing movements.

**Main firmware:** `ScribbleGrid.ino`

## Project

Developed as part of an undergraduate engineering design project at the University of Colorado Boulder.
