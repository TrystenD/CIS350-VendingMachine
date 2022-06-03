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
        Servo s;

    public:
        void AttachPin(int pin) {
            s.attach(pin);
        }

        void StartDispensing(void) {
            s.write(180);
        }

        void StopDispensing(void) {
            s.write(90);
        }
};


#define mid_pin   10
#define left_pin  11
#define right_pin 12

Item_Dispenser mid, left, right;
IR_Sensor ir;

void setup() {
  Serial.begin(9600);
  mid.AttachPin(mid_pin);
  left.AttachPin(left_pin);
  right.AttachPin(right_pin);
  
}

void loop() {
  left.StartDispensing();
  while (ir.DetectObject() == false);
  left.StopDispensing();
  
  delay(1000);
}
