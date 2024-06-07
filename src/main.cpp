/*

bottom view

   ____________________________________
1  | +5V - Vin          CLK           |
2  | CMD - NC           S00           |
3  | S03 - NC           S01           |
4  | S02 - NC           G15           |
5  | G13 - Start        G02           |
6  | GND                G00           |
7  | G12 - LED Data     G04           |
8  | G14 - NC           G16 - Blue
9  | G27 - red          G17 - Yellow
10 | G26 - NC           G05           |
11 | G25 - SPK          G18           |
12 | G33 - NC           G19           |
13 | G32 - green        GND           |
14 | G35 - NC           G21           |
15 | G34 - NC           RXD           |
16 |  SN - NC           TXD           |
17 |  SP - NC           G22           |
18 |  EN - NC           G23           |
19 |  3.3V              GND           |
   |__________________________________|

to the outside:
6 Red - orange/white -> rode knop
5 Green - green/white -> gele knop
4 Blue - blue/white -> groene knop
3 Yellow -brown/white -> blauwe knop
8 GND - brown
1 Speaker - blue
7 +5V - orange- green

*/


#include <Arduino.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <TimerEvent.h>

#define DRAG_COEFFICIENT 0.1
#define WEDSTRIJDRONDES 3
#define AANTAL_SPELERS 4

#define TONE_PIN 25
#define STARTBUTTON_PIN 13

hw_timer_t* timer = NULL;
bool sound=true;
bool soundon=true;

void IRAM_ATTR onTimer() {
  
  if (soundon) {
    digitalWrite(TONE_PIN, sound); 
    sound = !sound;
  }
}

Bounce2::Button start = Bounce2::Button();

TimerEvent game;
TimerEvent LedstringUpdate;
TimerEvent EndGame;

int RacePos=1;
bool Wait_for_start=true;


struct horse {
  private:
    Bounce2::Button button = Bounce2::Button();
    float position;
  
  public:

    uint16_t color;
    float speed;  
    int Int_position;
    int ronde;
    int Attachedpin;
    int place;
    bool finished=false;

  void updateposition() {
    position+=speed;
  }
  void reset() {
    speed=0;
    position=0;
    Int_position=0;
    ronde=0;
    place=0;
    finished=false;
    
  }
  void pinattach(int pin) {
    
    button.attach( pin, INPUT_PULLUP ); 
    button.interval(5); 
    Attachedpin=pin;
    button.setPressedState(LOW); 

  }

  bool update() {
    
    if (!finished) {
      button.update();
      if (button.pressed()) {
        speed+=0.2;
      }
      position+=speed;
      if (position>300) {
        position-=300;
        ronde++;
        if (ronde==WEDSTRIJDRONDES) {
          finished=true;
        } 
      }
    Int_position=position;
    speed=speed-speed*speed*DRAG_COEFFICIENT;
    return finished;
    } else return false;
    
  } 


};

void PlayGame();
void LedstringVisualize();
void GameReset();
void PlayTone(int frequency, int duration);

void play_starttones();
 
horse player[4];

const uint16_t horsecolors[]= {HUE_BLUE,HUE_GREEN,HUE_YELLOW,HUE_RED};
const uint16_t horsepins[]= {27,32,17,16};

#define NUM_LEDS 301
#define DATA_PIN 12
#define CLOCK_PIN -1

// Define the array of leds
CRGB leds[NUM_LEDS];

// put function declarations here:
void fadeall();




void setup() {
  // put your setup code here, to run once:
  FastLED.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
	FastLED.setBrightness(84);
  for (int i=0; i<AANTAL_SPELERS; i++) {
  player[i].pinattach(horsepins[i]);
  player[i].color=horsecolors[i];
    
  }

  pinMode(TONE_PIN, OUTPUT); 
  timer = timerBegin(3, 80, true);//div 80
  timerAttachInterrupt(timer, &onTimer, true);

  game.set(10,PlayGame);
  LedstringUpdate.set(20,LedstringVisualize);
  EndGame.disable();

  //play_starttones();
  start.attach(STARTBUTTON_PIN,INPUT_PULLUP);
start.interval(5);
}

void loop() {



  game.update();
  LedstringUpdate.update();
  EndGame.update();
  if (Wait_for_start) {
    start.update();
    if (start.pressed()) {
      Wait_for_start=false;
      play_starttones();
      game.enable();
    }
  }

 
   
}

// put function definitions here:
void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(180); 
  } 
}

void PlayGame() { //rondes not used yet

  // bool winner=false;

  for (int i=0;i<AANTAL_SPELERS;i++) {
    if(player[i].update()) {
      if (RacePos==1) EndGame.set(30000,GameReset);
      player[i].place=RacePos++;
      if (RacePos > AANTAL_SPELERS) {
        EndGame.set(10000,GameReset);
        game.disable();
      }
    };
    //leds[player[i].Int_position] = CHSV(horsecolors[i], 255, 255);
    //if (player[i].finished) winner=true;
  }
  //fadeall();
  //FastLED.show();
  //return winner;
}

void LedstringVisualize() {
  leds[0]=CRGB::White;
  for (int i=0;i<AANTAL_SPELERS;i++) {
    
    if (player[i].place > 0) {
      for (int j=0;j<5;j++) {
        
        leds[30-player[i].place*5-j]= CHSV(player[i].color, 255, 255);

      }
    } //else leds[player[i].Int_position+1] = CHSV(player[i].color, 255, 255);
  }
  for (int i=0;i<AANTAL_SPELERS;i++) {
     if (player[i].place == 0) {
      leds[player[i].Int_position+1] = CHSV(player[i].color, 255, 255);
     }

  }
   
   
  fadeall();
  FastLED.show();
}

void GameReset() {
  game.disable();
  for (int i=0;i<AANTAL_SPELERS;i++) {
    player[i].reset();
  }
  RacePos=1;
  Wait_for_start=true;
  EndGame.disable();
}

void PlayTone(int frequency, int duration) {

    timerAlarmDisable(timer);
    timerAlarmWrite(timer, 1000000l / frequency, true);
    timerAlarmEnable(timer);
    soundon=true;
    delay(duration);
    soundon=false;
    digitalWrite(TONE_PIN,false);
    timerAlarmDisable(timer);
}

void play_starttones() { 
  PlayTone(660,400);
  delay(200);
  PlayTone(660,400);
  delay(200);
  PlayTone(1320,1200);

}

