#include <Servo.h>

class IR_Sensor {

    private:
      int ir_pin = A5;

    public:
      boolean DetectObject(void) {
        return analogRead(ir_pin) > 350;
      }
      
};


class Item_Dispenser {
  
    private:
        Servo servo;

    public:
        void AttachPin(int pin) {
            servo.attach(pin);
        }

        void StartDispensing(void) {
            servo.write(180);
        }

        void StopDispensing(void) {
            servo.write(90);
        }
};


#define DISPENSER_1_PIN   10
#define DISPENSER_2_PIN  11
#define DISPENSER_3_PIN 12

Item_Dispenser dispenser1, dispenser2, dispenser3;
IR_Sensor ir;

void setup() {
  Serial.begin(9600);
  dispenser1.AttachPin(DISPENSER_1_PIN);
  dispenser2.AttachPin(DISPENSER_2_PIN);
  dispenser3.AttachPin(DISPENSER_3_PIN);
  
}

void loop() {
  dispenser2.StartDispensing();
  while (ir.DetectObject() == false);
  dispenser2.StopDispensing();
  
  delay(1000);
}
