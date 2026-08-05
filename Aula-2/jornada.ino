const int S2 = 27;
const int S3 = 26;
const int sensorOut = 13;

void setup() {
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  Serial.begin(9600);
}

void loop() {
  long sumR = 0, sumG = 0, sumB = 0;

  for (int i = 0; i < 10; i++) {
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    sumR += pulseIn(sensorOut, LOW);

    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    sumG += pulseIn(sensorOut, LOW);

    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    sumB += pulseIn(sensorOut, LOW);

    delay(100);
  }

  Serial.print(sumR / 10);
  Serial.print(",");
  Serial.print(sumG / 10);
  Serial.print(",");
  Serial.println(sumB / 10);

  delay(1000);
}