
void setup() {
    
    Serial.begin(0);
    while(!Serial);
    
    Serial.println("Start test2");
    
}

void loop() {
    
    Serial.println("loop");
    delay(2000);
    
}