String[] lines;

import hypermedia.net.*;

 UDP udp;  // define the UDP object
 
  String ip       = "192.168.2.177";  // the remote IP address
 int port        = 8050;    // the destination port

int index = 0;

 void setup() {
 udp = new UDP( this, 8050 );  // create a new datagram connection on port 6000
 //udp.log( true );     // <-- printout the connection activity
 udp.listen( true );           // and wait for incoming message
 lines = loadStrings("Blink.ino.hex");
 }

 void draw()
 {
 }

 void keyPressed() {
   if (key == 'u') {
     index = 0;
     udp.send("firmware_update", ip, port );   // the message to send
   }else if (key == 'f') {
     udp.send("firmware_line__" + ":flash "+ str(lines.length) + '\n',  ip, port );
     print(":flash ");
    print(str(lines.length));
    print("\n");
   }
 }

 void receive( byte[] data ) {       // <-- default handler
 //void receive( byte[] data, String ip, int port ) {  // <-- extended handler
 if (data[0] == 10) {
   if (index < lines.length) {
     print("sending new line");
     String[] pieces = split(lines[index], '\n');
     udp.send("firmware_line__" + pieces[0] + '\n', ip, port );   // 
      print(pieces[0]);
      print("\t");
      print(index+1);
      print(" of ");
      print(lines.length);
      print("\t");
      //delay(5);
      // Go to the next line for the next run through draw()
      index++;
   } else {
     delay(100);
     udp.send(":flash "+ str(lines.length) + '\n',  ip, port );
     print(":flash ");
    print(str(lines.length));
    print("\n");
   }
 }
 println();
 
 }