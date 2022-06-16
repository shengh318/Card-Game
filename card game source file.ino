#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <ArduinoJson.h>




const uint8_t BETTING = 0; //example definition
const uint8_t DRAWING = 1;
const uint8_t PLAYER_AUTO_WIN = 2;
const uint8_t GAME = 3;
const uint8_t PLAYER_BET = 4;
const uint8_t BANKER_BET = 5;
const uint8_t BANKER_AUTO_WIN = 6;
const uint8_t PLAYER_WIN = 7;
const uint8_t BANKER_WIN = 8;
const uint8_t TIE = 9;
const uint8_t NO_MONEY = 10;

uint8_t state;
//const uint8_t UP = 2; //change if you want!
//add more if you want!!!

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 2000; //periodicity of getting a number fact.
const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char new_response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char banker_response_buffer[OUT_BUFFER_SIZE];

char host[] = "deckofcardsapi.com";
const char* deck_id;

char network[] = "EECS_Labs";
char password[] = "";

uint8_t scanning = 0;//set to 1 if you'd like to scan for wifi networks (see below):
/* Having network issues since there are 50 MIT and MIT_GUEST networks?. Do the following:
    When the access points are printed out at the start, find a particularly strong one that you're targeting.
    Let's say it is an MIT one and it has the following entry:
   . 4: MIT, Ch:1 (-51dBm)  4:95:E6:AE:DB:41
   Do the following...set the variable channel below to be the channel shown (1 in this example)
   and then copy the MAC address into the byte array below like shown.  Note the values are rendered in hexadecimal
   That is specified by putting a leading 0x in front of the number. We need to specify six pairs of hex values so:
   a 4 turns into a 0x04 (put a leading 0 if only one printed)
   a 95 becomes a 0x95, etc...
   see starting values below that match the example above. Change for your use:
   Finally where you connect to the network, comment out 
     WiFi.begin(network, password);
   and uncomment out:
     WiFi.begin(network, password, channel, bssid);
   This will allow you target a specific router rather than a random one!
*/
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.


const int PLAYER_BET_BUTTON = 45; //pin connected to button 
const int BANKER_BET_BUTTON = 39;
const int DRAW_3rd_CARD_BUTTON = 34;
const int NEW_GAME_BUTTON = 38;

void setup(){
  tft.init();  //init screen
  tft.setRotation(3); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms
  if (scanning){
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  pinMode(PLAYER_BET_BUTTON, INPUT_PULLUP); //set input pin as an input!
  pinMode(BANKER_BET_BUTTON, INPUT_PULLUP); //set input pin as an input!
  pinMode(DRAW_3rd_CARD_BUTTON, INPUT_PULLUP); //set input pin as an input!
  pinMode(NEW_GAME_BUTTON, INPUT_PULLUP); //set input pin as an input!
  state = BETTING;
}



/*------------------
 * number_fsm Function:
 * Use this to implement your finite state machine. It takes in an input (the reading from a switch), and can use
 * state as well as other variables to be your state machine.
 */



void drawDeck(){


  sprintf(request_buffer, "GET http://deckofcardsapi.com/api/deck/new/shuffle/?deck_count=1 HTTP/1.1\r\n");

  strcat(request_buffer, "Host: deckofcardsapi.com\r\n"); //add more to the end

  strcat(request_buffer, "\r\n"); //add blank line!

  //submit to function that performs GET.  It will return output using response_buffer char array

  do_http_GET(host, request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

  //Serial.println(response_buffer); //print to serial monitor

  //deckID = response_buffer["deck_id"];
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response_buffer);
  deck_id = doc["deck_id"];

}

const char* cards;
const char* player_three_cards;
const char* banker_three_cards;

const char* player_1_card;
const char* player_2_card;
const char* banker_1_card;
const char* banker_2_card;

const char* player_3_card;
const char* banker_3_card;

int remaining_card_count = 52;

void drawCards(){
  sprintf(request_buffer, "GET http://deckofcardsapi.com/api/deck/%s/draw/?count=4 HTTP/1.1\r\n", deck_id);

  strcat(request_buffer, "Host: deckofcardsapi.com\r\n"); //add more to the end

  strcat(request_buffer, "\r\n"); //add blank line!

  do_http_GET(host, request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

  //Serial.println(response_buffer); //print to serial monitor

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response_buffer);
  cards = doc["cards"][0]["value"];
  //Serial.println(cards);

  remaining_card_count = doc["remaining"];

  
  player_1_card = doc["cards"][0]["value"];
  player_2_card = doc["cards"][1]["value"];
  banker_1_card = doc["cards"][2]["value"];
  banker_2_card = doc["cards"][3]["value"];

  
}

void player_draw_one_card(const char* player1_card, const char* player2_card, const char* banker1_card, const char* banker2_card){

  player_1_card = player1_card;
  player_2_card = player2_card;
  banker_1_card = banker1_card;
  banker_2_card = banker2_card;

  sprintf(request_buffer, "GET http://deckofcardsapi.com/api/deck/%s/draw/?count=1 HTTP/1.1\r\n", deck_id);

  strcat(request_buffer, "Host: deckofcardsapi.com\r\n"); //add more to the end

  strcat(request_buffer, "\r\n"); //add blank line!

  do_http_GET(host, request_buffer, new_response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

  //Serial.println(response_buffer); //print to serial monitor

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, new_response_buffer);
  player_three_cards = doc["cards"][0]["value"];

  remaining_card_count = doc["remaining"];
  
  player_3_card = player_three_cards;

  
}

void banker_draw_one_card(const char* player1_card, const char* player2_card, const char* banker1_card, const char* banker2_card){

  player_1_card = player1_card;
  player_2_card = player2_card;
  banker_1_card = banker1_card;
  banker_2_card = banker2_card;

  sprintf(request_buffer, "GET http://deckofcardsapi.com/api/deck/%s/draw/?count=1 HTTP/1.1\r\n", deck_id);

  strcat(request_buffer, "Host: deckofcardsapi.com\r\n"); //add more to the end

  strcat(request_buffer, "\r\n"); //add blank line!

  do_http_GET(host, request_buffer, banker_response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

  //Serial.println(response_buffer); //print to serial monitor

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, banker_response_buffer);
  banker_three_cards = doc["cards"][0]["value"];

  remaining_card_count = doc["remaining"];
  
  banker_3_card = banker_three_cards;

  
}


/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c: 
 *    uint16_t buff_size: size of buffer buff
 *    
 * Return value: 
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

/*----------------------------------
 * do_http_GET Function:
 * Arguments:
 *    char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP GET request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */
void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){

  WiFiClient client; //instantiate a client object

  if (client.connect(host, 80)) { //try to connect to host on port 80

    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces

    client.print(request);

    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer

    uint32_t count = millis();

    while (client.connected()) { //while we remain connected read out data coming back

      client.readBytesUntil('\n',response,response_size);

      if (serial) Serial.println(response);

      if (strcmp(response,"\r")==0) { //found a blank line!

        break;

      }

      memset(response, 0, response_size);

      if (millis()-count>response_timeout) break;

    }

    memset(response, 0, response_size); 

    count = millis();

    while (client.available()) { //read out remaining text (body of response)

      char_append(response,client.read(),OUT_BUFFER_SIZE);

    }

    if (serial) Serial.println(response);

    client.stop();

    if (serial) Serial.println("-----------"); 

  }else{

    if (serial) Serial.println("connection failed :/");

    if (serial) Serial.println("wait 0.5 sec...");

    client.stop();

  }

}           

bool game_play = true;



int check_value_of_card(const char* card){
  // Serial.println(card);
  // Serial.println("---------");
  if (strcmp(card, "ACE")==0){
    //Serial.println("IN ACE");
    return 1;
  }
  else if (strcmp(card, "10")==0){
    //Serial.println("IN 10");
    return 0;
  }
  else if (strcmp(card, "KING")==0){
    //Serial.println("IN KING");
    return 0;
  }
  else if (strcmp(card, "QUEEN")==0){
    //Serial.println("IN QUEEN");
    return 0;
  }
  else if (strcmp(card, "JACK")==0){
    //Serial.println("IN JACK");
    return 0;
  }
  else{

    //Serial.println("IN REGULAR");
    return atoi(card);
  }
}



int player_winnings = 0;

int player_money = 100;

bool start = true;
int player_bets=0;
int banker_bets=0;

bool first_game = true;
bool first_deck = true;

int player_total;
int banker_total;
bool player_auto_win = true;
bool banker_auto_win = true;

bool player_win = false;
bool banker_win = false;
bool tie = false;

void loop(){

  uint8_t player_bet_button = digitalRead(PLAYER_BET_BUTTON);
  uint8_t banker_bet_button = digitalRead(BANKER_BET_BUTTON);
  uint8_t draw_card_button = digitalRead(DRAW_3rd_CARD_BUTTON);
  uint8_t new_game_button = digitalRead(NEW_GAME_BUTTON);

  switch (state){
    
    case BETTING:
      if (first_deck){
        drawDeck();
        first_deck = false;
      }

      if (remaining_card_count < 6){
        drawDeck();
      }

      first_game = true;
      if (start){
        print_out_betting_screen(player_bets, banker_bets); 
        start = false;
      }

      if (player_bet_button == 0){
        Serial.println("GOING TO PLAYER BET");
        state = PLAYER_BET;
      }

      else if (banker_bet_button == 0){
        Serial.println("GOING TO BANKER BET");
        state = BANKER_BET;
      }

      else if (draw_card_button == 0){
        Serial.println("GOING TO GAME SCREEN");
        state = GAME;
      }
    break;

    case PLAYER_BET:
    
      if (player_bet_button == 1){
        if (player_money > 0){
          Serial.println("PLAYER BET");
          player_bets += 1;
          
          player_money -=10;

          start = true;
        }
        state = BETTING;
      }
      
    break;
    
    case BANKER_BET:
      
      if (banker_bet_button == 1){
        if (player_money > 0){
          Serial.println("BANKER BET");
          banker_bets += 1;
          player_money -=10;
          start = true;       
        } 
        state = BETTING;
      }
    break;

    case GAME:
      if (first_game){
        Serial.println("IN FIRST GAME");
        drawCards();
        print_out_game_screen(player_1_card, player_2_card, banker_1_card, banker_2_card);

        //Serial.println(deck_id);
        first_game = false;

        //player_total = 3;

        if ((player_total == 8 && banker_total == 8) || (player_total == 9 && banker_total == 9)){
          tie = true;
          state = TIE;
        }

        else if (player_total == 8 || player_total == 9){
          player_auto_win = true;
          state = PLAYER_AUTO_WIN;
        }

        else if (banker_total == 8 || banker_total == 9){
          banker_auto_win = true;
          state = BANKER_AUTO_WIN;
        }

        else{
          if (player_total >= 0 && player_total <= 5){

            player_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
            update_player_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card);
            


            //banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);

            //update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);

            if (strcmp(player_3_card, "KING") == 0 || strcmp(player_3_card, "QUEEN")==0 || strcmp(player_3_card, "JACK")==0){
              if (banker_total >= 0 && banker_total <= 3){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "ACE")==0){
              if (banker_total >= 0 && banker_total <= 3){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "9")==0 || strcmp(player_3_card, "10") == 0){
              if (banker_total >= 0 && banker_total <= 3){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "8") == 0){
              if (banker_total >= 0 && banker_total <= 2){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "6") == 0 || strcmp(player_3_card, "7") == 0){
              if (banker_total >= 0 && banker_total <= 6){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "4") == 0 || strcmp(player_3_card, "5") == 0){
              if (banker_total >= 0 && banker_total <= 5){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }

            else if (strcmp(player_3_card, "2") == 0 || strcmp(player_3_card, "3") == 0){
              if (banker_total >= 0 && banker_total <= 4){
                banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
                update_both_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card, banker_3_card);
              }
            }
          }
          else{
            if (banker_total >= 0 && banker_total <= 5){
              Serial.println("IN ONLY BANKER DRAW");
              banker_draw_one_card(player_1_card, player_2_card, banker_1_card, banker_2_card);
              update_banker_game_screen(player_1_card, player_2_card, banker_3_card, banker_1_card, banker_2_card);
            }
          }

          check_win(player_total, banker_total);

          if (player_win){
            state = PLAYER_WIN;
          }

          else if (banker_win){
            state = BANKER_WIN;
          }

          else{
            state = TIE;
          }
            //update_player_game_screen(player_1_card, player_2_card, player_3_card, banker_1_card, banker_2_card);
            //update_banker_game_screen(player_1_card, player_2_card, banker_3_card, banker_1_card, banker_2_card);

          



        }

      }

    break;


    case TIE:

      if (tie){
        player_win = false;
        banker_win = false;
        tie = false;

        player_money += banker_bets * 10;
        player_money += player_bets * 10;

        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;

        tft.println("--------------------");
        tft.println("TIE!!!!!");
        tft.println("BUTTON 4 to CONTINUE");

      }

      if (new_game_button == 0){
        Serial.println("GOING TO BETTING");        
        state = BETTING;
        start = true;
      }

    break;

    case BANKER_WIN:
      if (banker_win){
        player_win = false;
        banker_win = false;
        tie = false;

        player_money += banker_bets * 20;
        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;

        tft.println("--------------------");
        tft.println("BANKER WON!!!!!");
        tft.println("BUTTON 4 to CONTINUE");
      }

      if (player_money == 0){
        Serial.println("GOING TO NO MONEY");
        state = NO_MONEY;
      }

      if (new_game_button == 0){
        Serial.println("GOING TO BETTING");        
        state = BETTING;
        start = true;
      }

    break;

    case PLAYER_WIN:
      if (player_win){
        player_win = false;
        banker_win = false;
        tie = false;

        player_money += player_bets * 20;
        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;

        tft.println("--------------------");
        tft.println("PLAYER WON!!!!!");
        tft.println("BUTTON 4 to CONTINUE");
      }

      if (player_money == 0){
        Serial.println("GOING TO NO MONEY");
        state = NO_MONEY;
      }

      if (new_game_button == 0){
        Serial.println("GOING TO BETTING");        
        state = BETTING;
        start = true;
      }

    break;

    case PLAYER_AUTO_WIN:
      if (player_auto_win){
        tft.println("--------------------");
        tft.println("PLAYER WON!!!!!");
        tft.println("BUTTON 4 to CONTINUE");
        player_money += player_bets*20;
        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;
        player_auto_win = false;
        
      }

      if (player_money == 0){
        Serial.println("GOING TO NO MONEY");
        state = NO_MONEY;
      }

      if (new_game_button == 0){
          Serial.println("GOING TO BETTING");        
          state = BETTING;
          start = true;
      }
    break;

    case BANKER_AUTO_WIN:
      if (banker_auto_win){
        tft.println("--------------------");
        tft.println("BANKER WON!!!!!");
        tft.println("BUTTON 4 to CONTINUE");
        player_money += banker_bets*20;
        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;
        banker_auto_win = false;
        
      }
      
      if (player_money <= 0){
        Serial.println("GOING TO NO MONEY");
        state = NO_MONEY;
      }

      if (new_game_button == 0){
          Serial.println("GOING TO BETTING");        
          state = BETTING;
          start = true;
      }

    break;

    case NO_MONEY:
      if (player_money == 0){
        no_money_screen();
        player_money = 100;
        player_total = 0;
        banker_total = 0;
        player_bets = 0;
        banker_bets = 0;
      }

      if (new_game_button == 0){
        Serial.println("GOING TO BETTING");
        state = BETTING;
        start = true;
      }
    break;

  }
  if (game_play){
    
    //drawCards();
    //print_out_game_screen(player_1_card, player_2_card, banker_1_card, banker_2_card);
    
    game_play = false;
  }

}

void no_money_screen(){
  tft.println("-------------------");
  tft.println("YOU RAN OUT OF MONEY!!!!!");
}

void update_both_game_screen(const char* player_1_card, const char* player_2_card, const char* player_3_card, const char* banker_1_card, const char* banker_2_card, const char* banker_3_card ){
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setCursor(0,0,1);
  tft.println("GAME");
  tft.println("---------------");

  int player_1_value;
  int player_2_value;
  int banker_1_value;
  int banker_2_value;

  int player_3_value;
  int banker_3_value;

  player_1_value = check_value_of_card(player_1_card);
  player_2_value = check_value_of_card(player_2_card);
  player_3_value = check_value_of_card(player_3_card);

  banker_1_value = check_value_of_card(banker_1_card);
  banker_2_value = check_value_of_card(banker_2_card);
  banker_3_value = check_value_of_card(banker_3_card);

  player_total = player_1_value + player_2_value + player_3_value;
  banker_total = banker_1_value + banker_2_value + banker_3_value;

  if (player_total > 9){
    player_total = player_total % 10;
  }

  if (banker_total > 9){
    banker_total = banker_total % 10;
  }

  char player_cards[700];
  sprintf(player_cards, "Player Cards: ");
  strcat(player_cards, player_1_card);
  strcat(player_cards, ", ");
  strcat(player_cards, player_2_card);
  strcat(player_cards, ", ");
  strcat(player_cards, player_3_card);
  Serial.println(player_cards);
  tft.println(player_cards);

  
  char banker_cards[700];
  sprintf(banker_cards, "Banker Cards: ");
  strcat(banker_cards, banker_1_card);
  strcat(banker_cards, ", ");
  strcat(banker_cards, banker_2_card);
  strcat(banker_cards, ", ");
  strcat(banker_cards, banker_3_card);
  Serial.println(banker_cards);
  tft.println(banker_cards);

  print_out_player_total(player_total);
  print_out_banker_total(banker_total);

  char remaining_card[100];
  sprintf(remaining_card, "Remaining Cards: %d", remaining_card_count );
  tft.println(remaining_card);

}

void check_win(int player_total, int banker_total){

  int player_difference;
  int banker_difference;

  if (player_total > 9){
    player_difference = player_total - 9;
  }
  else{
    player_difference = 9-player_total;
  }

  if (banker_total > 9){
    banker_difference = banker_total - 9;
  }
  else{
    banker_difference = 9-banker_total;
  }

  if (player_difference > banker_difference){
    banker_win = true;
    player_win = false;
  }
  else if (banker_difference > player_difference){
    player_win = true;
    banker_win = false;
  }
  else{
    player_win = false;
    banker_win = false;
    tie = true;
  }
  
}

void update_banker_game_screen(const char* player_1_card, const char* player_2_card, const char* banker_3_card, const char* banker_1_card, const char* banker_2_card){
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setCursor(0,0,1);
  tft.println("GAME");
  tft.println("---------------");

  int player_1_value;
  int player_2_value;
  int banker_1_value;
  int banker_2_value;

  int banker_3_value;

  player_1_value = check_value_of_card(player_1_card);
  player_2_value = check_value_of_card(player_2_card);


  banker_1_value = check_value_of_card(banker_1_card);
  banker_2_value = check_value_of_card(banker_2_card);
  banker_3_value = check_value_of_card(banker_3_card);

  player_total = player_1_value + player_2_value;
  banker_total = banker_1_value + banker_2_value + banker_3_value;


  if (banker_total > 9){
    banker_total = banker_total % 10;
  }

  print_out_player_cards(player_1_card, player_2_card);
  
  char banker_cards[700];
  sprintf(banker_cards, "Banker Cards: ");
  strcat(banker_cards, banker_1_card);
  strcat(banker_cards, ", ");
  strcat(banker_cards, banker_2_card);
  strcat(banker_cards, ", ");
  strcat(banker_cards, banker_3_card);
  Serial.println(banker_cards);
  tft.println(banker_cards);

  print_out_player_total(player_total);
  print_out_banker_total(banker_total);

  char remaining_card[100];
  sprintf(remaining_card, "Remaining Cards: %d", remaining_card_count );
  tft.println(remaining_card);

}


void update_player_game_screen(const char* player_1_card, const char* player_2_card, const char* player_3_card, const char* banker_1_card, const char* banker_2_card){
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setCursor(0,0,1);
  tft.println("GAME");
  tft.println("---------------");

  int player_1_value;
  int player_2_value;
  int banker_1_value;
  int banker_2_value;

  int player_3_value;

  player_1_value = check_value_of_card(player_1_card);
  player_2_value = check_value_of_card(player_2_card);
  player_3_value = check_value_of_card(player_3_card);

  banker_1_value = check_value_of_card(banker_1_card);
  banker_2_value = check_value_of_card(banker_2_card);
  
  player_total = player_1_value + player_2_value + player_3_value;
  banker_total = banker_1_value + banker_2_value;

  if (player_total > 9){
    player_total = player_total % 10;
  }

  if (banker_total > 9){
    banker_total = banker_total % 10;
  }

  char player_cards[700];
  sprintf(player_cards, "Player Cards: ");
  strcat(player_cards, player_1_card);
  strcat(player_cards, ", ");
  strcat(player_cards, player_2_card);
  strcat(player_cards, ", ");
  strcat(player_cards, player_3_card);
  Serial.println(player_cards);
  tft.println(player_cards);

  print_out_banker_cards(banker_1_card, banker_2_card);

  
  print_out_player_total(player_total);
  print_out_banker_total(banker_total);

  char remaining_card[100];
  sprintf(remaining_card, "Remaining Cards: %d", remaining_card_count );
  tft.println(remaining_card);

}
void print_out_betting_screen(int player_counter, int banker_counter){
  char player_betting[200];
  char banker_betting[200];
  char player_money_string[200];

  sprintf(player_betting, "Player Bets: %d", player_counter*10);
  sprintf(banker_betting, "Banker Bets: %d", banker_counter*10);
  sprintf(player_money_string, "Player Money: %d", player_money);

  tft.fillScreen(TFT_BLACK); //fill background
  tft.setCursor(0,0,1);
  tft.println("BETTING");
  tft.println("---------------");
  tft.println(player_betting);
  tft.println(banker_betting);
  tft.println("---------------");
  tft.println(player_money_string);
  tft.println("---------------");  
  tft.println("");
  tft.println("BUTTON 1 FOR PLAYER");
  tft.println("BUTTON 2 FOR BANKER");
  tft.println("BUTTON 3 WHEN READY");

}

void print_out_game_screen(const char* player_1_card, const char* player_2_card, const char* banker_1_card, const char* banker_2_card){
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setCursor(0,0,1);
  tft.println("GAME");
  tft.println("---------------");


  int player_1_value;
  int player_2_value;
  int banker_1_value;
  int banker_2_value;

  player_1_value = check_value_of_card(player_1_card);
  player_2_value = check_value_of_card(player_2_card);
  banker_1_value = check_value_of_card(banker_1_card);
  banker_2_value = check_value_of_card(banker_2_card);
  
  player_total = player_1_value + player_2_value;
  banker_total = banker_1_value + banker_2_value;

  if (player_total > 9){
    player_total = player_total%10;
  }

  if (banker_total > 9){
    banker_total = banker_total % 10;
  }

  print_out_player_cards(player_1_card, player_2_card);
  print_out_banker_cards(banker_1_card, banker_2_card);

  
  print_out_player_total(player_total);
  print_out_banker_total(banker_total);

  char remaining_card[100];
  sprintf(remaining_card, "Remaining Cards: %d", remaining_card_count );
  tft.println(remaining_card);

}

void print_out_banker_total(int total){
  char banker_total_string[100];
  sprintf(banker_total_string, "Banker Total: %d", total);
  tft.println(banker_total_string);
}

void print_out_player_total(int total){
  char player_total_string[100];
  sprintf(player_total_string, "Player Total: %d", total);
  tft.println(player_total_string);
}

void print_out_player_cards(const char* card_one, const char* card_two){
  //strcat("Player cards: ", card_one, " ", card_two);
  char player_cards[500];
  sprintf(player_cards, "Player Cards: ");
  strcat(player_cards, card_one);
  strcat(player_cards, ", ");
  strcat(player_cards, card_two);
  Serial.println(player_cards);
  tft.println(player_cards);
}

void print_out_banker_cards(const char* card_one, const char* card_two){
  //strcat("Player cards: ", card_one, " ", card_two);
  char banker_cards[500];
  sprintf(banker_cards, "Banker Cards: ");
  strcat(banker_cards, card_one);
  strcat(banker_cards, ", ");
  strcat(banker_cards, card_two);
  Serial.println(banker_cards);
  tft.println(banker_cards);
}
