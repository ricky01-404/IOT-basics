#define trigPin 7
#define echoPin 6
#define led1 12
#define led2 13
#define buzzer 11

void setup() {
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  long duration, distance;
  digitalWrite(trigPin, LOW);
  digitalWrite(trigPin, HIGH);

  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  if (distance < 100) 
  {  
    digitalWrite(led1,LOW); 
  digitalWrite(led2,HIGH);
  digitalWrite(buzzer,LOW);
  
  
}
  if (distance <50) {
    digitalWrite(led1,HIGH);
    digitalWrite(led2,LOW);
    digitalWrite(buzzer,LOW);
  
    
  }
  if (distance < 15) {
    digitalWrite(led1,HIGH);
    digitalWrite(led2,HIGH);
    digitalWrite(buzzer,HIGH);
  
  
  }
  else {
    Serial.print(distance);
    Serial.println(" cm");
  }
  
}
