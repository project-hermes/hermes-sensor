void setup() {
    Particle.function("testdata", testData);
}

void loop() {
}

int testData(String command){
  Particle.publish("D",String::format("t%d T%d D%d",Time.now(),random(80,95),random(1000,60000)));
  return 0;
}
