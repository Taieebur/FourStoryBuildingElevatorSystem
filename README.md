# FourStoryBuildingElevatorSystem

### Pin Connections

- Pushbutton 1 to 4 on Arduino D1, D2, D4 and D5
- LED 1 to 4 on D6, D7, D8 and D9
- L293D pin1 on Arduino D3, pin2 on D12, pin3 on Lift Motor Positive, pin4 on power supply negative and display ground, pin5 on Arduino GND, pin6 on Lift Motor Negative, pin7 on D13, pin8 on power supply positive and regulator input, pin9 on 5V, pin10 on D10, pin11 on Door Motor Positive, pin14 on Door Motor Negative, pin15 on D11 and pin16 on 5V.
- Force sensor input on Arduino analog pin A3
- LDR input on A2
- Door closing limit switch input(bottom pushbutton) on A1
- Door opening limit switch input(adjacent bottom pushbutton) on A0
- Display SDA on A5
- Display SCL on A6



### Testing

- Run the simulation
- Click the bottom pushbutton to close door
- Select Target floor(Top 4 pushbuttons)
- The elevator will wait few seconds(2 seconds) before moving. Within that time you can adjust weight from the force sensor or change the LDR value before closing doors to simulate hands on doors
- Upon reaching the destination floor, door opening motor will start spinning. Hit the adjacent bottom pushbutton(limit switch alternative) to finish door opening process.
- After finishing the door opening process, it will start closing door and the door opening motor will start spinning in reverse direction. Hit the bottom pushbutton(limit switch alternative) to terminate door closing process.
- Upon closing the doors, the elevator will stay on the destination floor and the destination floor LED will stay turned on unless another call received.
