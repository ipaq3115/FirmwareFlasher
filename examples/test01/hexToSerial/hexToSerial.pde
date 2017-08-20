/**
 * Simple Read
 * 
 * Read data from the serial port and change the color of a rectangle
 * when a switch connected to a Wiring or Arduino board is pressed and released.
 * This example works with the Wiring / Arduino program that follows below.
 */


import processing.serial.*;

Serial myPort;  // Create object from Serial class
String val;      // Data received from the serial port
String[] lines;
int index = 0;
boolean req = false;

void setup() 
{
  size(200, 200);
  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.

  println(Serial.list()[0]);
  
  String portName = Serial.list()[0];
  myPort = new Serial(this, portName, 115200);
  lines = loadStrings("fromProcessing.ino.TEENSY31.hex");
}

void draw()
{
  if ( myPort.available() > 0) {  // If data is available,
    val = myPort.readStringUntil('\n');
    if (val == null) {
    }else if (val.contains("@") == true) {
      println("boot detect");
      myPort.write('@');
    }else if (val.contains("¤") == true) {
      println("sending HEX"); //<>//
      delay(1);
      req = true;
    }else if (val.contains("Y") == true) {
      if (req == true) {
        if (index < lines.length) {
          delay(200);
          String[] pieces = split(lines[index], '\n');
          myPort.write(pieces[0]);
          myPort.write('\n');
          print(pieces[0]);
          print("\t");
          print(index+1);
          print(" of ");
          print(lines.length);
          print("\t");
          //delay(5);
          // Go to the next line for the next run through draw()
          index = index + 1;
          if (index >= lines.length) {
            req = false;
            delay(10);
          }
        } //<>//
      }
    }else if (val.contains("waiting for :flash") == true) {
        //myPort.write("\n");
        myPort.write(":flash");
        myPort.write(str(lines.length));
        myPort.write("\n");
        print(":flash");
        print(str(lines.length));
        print("\n");
        index = index + 1;
      }
    print(val);
    val = null; //clear val
  }
}