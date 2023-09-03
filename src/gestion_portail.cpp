#include <Arduino.h>
#include "main_include.h"

#include <ezButton.h>
#define M1 0
#define M2 1
// #define PIN_K1_TBD
// #define PIN_K2_TBD
// 
// #define PIN_K3_SORTIE_LAMPE
// #define PIN_K4_OUVERTURE_M1
// #define PIN_K5_OUVERTURE_M2
// #define PIN_K6_FERMETURE_M1
// #define PIN_K7_FERMETURE_M2
// 
// #define PIN_IN_FCA1_OUVERTURE_M1
// #define PIN_IN_FCA2_OUVERTURE_M2
// #define PIN_IN_FCC1_FERMETURE_M1
// #define PIN_IN_FCC2_FERMETURE_M2
// #define PIN_IN_OUVERTURE_PIETON
// #define PIN_IN_START
// #define PIN_IN_STOP
// #define PIN_POT_TCA_TEMPS_FERMETURE_AUTO
// #define PIN_POT_TLAV_TEMPS_TRAVAIL
// #define PIN_POT_TSFAS_TEMPS_DECALAGE_FERMETURE

uint8_t led_K1_TBD          = PIN_K1_TBD         ;
uint8_t led_K2_TBD          = PIN_K2_TBD         ;
uint8_t led_K3_sortie_lampe = PIN_K3_SORTIE_LAMPE;
uint8_t led_K4_ouverture_M1 = PIN_K4_OUVERTURE_M1;
uint8_t led_K5_ouverture_M2 = PIN_K5_OUVERTURE_M2;
uint8_t led_K6_fermeture_M1 = PIN_K6_FERMETURE_M1;
uint8_t led_K7_fermeture_M2 = PIN_K7_FERMETURE_M2;
//ezButton btn_FDC_open_M1      = { PIN_IN_FCA1_OUVERTURE_M1 };
//ezButton btn_FDC_open_M2      = { PIN_IN_FCA2_OUVERTURE_M2 };
//ezButton btn_FDC_close_M1     = { PIN_IN_FCC1_FERMETURE_M1 };
//ezButton btn_FDC_close_M2     = { PIN_IN_FCC2_FERMETURE_M2 };
ezButton btn_ouverture_pieton = { PIN_IN_OUVERTURE_PIETON , INPUT_PULLUP};
ezButton btn_start            = { PIN_IN_START            , INPUT_PULLUP};
ezButton btn_stop             = { PIN_IN_STOP             , INPUT_PULLUP};

ezButton btn_FDC_open[2] = {    ezButton(PIN_IN_FCA1_OUVERTURE_M1, INPUT_PULLUP),
                                ezButton(PIN_IN_FCA2_OUVERTURE_M2, INPUT_PULLUP)
};
ezButton btn_FDC_close[2] = {   ezButton(PIN_IN_FCC1_FERMETURE_M1, INPUT_PULLUP),
                                ezButton(PIN_IN_FCC2_FERMETURE_M2, INPUT_PULLUP)
};

static unsigned long t0_state[2] = {0, 0} ;  // pour timing ouverture/fermeture  : temps initial
//static unsigned long delay_wait_opening[2] = {0, 0} ;  // pour timing ouverture/fermeture  : temps initial

unsigned long  temps_delai_ouverture_ms[2] = {2000, 4000}; 
unsigned long  temps_delai_fermeture_ms[2] = {2000, 4000}; 

unsigned long  temps_fermeture_auto_ms    ; 
unsigned long  temps_travail_ms           ; 
unsigned long  temps_decalage_fermeture_ms; 


// setup des E/S 
void setup_inputs_and_ouputs(void)
{
    pinMode(led_K1_TBD         , OUTPUT);
    pinMode(led_K2_TBD         , OUTPUT);
    pinMode(led_K3_sortie_lampe, OUTPUT);
    pinMode(led_K4_ouverture_M1, OUTPUT);
    pinMode(led_K5_ouverture_M2, OUTPUT);
    pinMode(led_K6_fermeture_M1, OUTPUT);
    pinMode(led_K7_fermeture_M2, OUTPUT);

//    pinMode(btn_FDC_open_M1     .pin, INPUT);
//    pinMode(btn_FDC_open_M2     .pin, INPUT);
//    pinMode(btn_FDC_close_M1    .pin, INPUT);
//    pinMode(btn_FDC_close_M2    .pin, INPUT);
//    pinMode(btn_ouverture_pieton.pin, INPUT);
//    pinMode(btn_start           .pin, INPUT);
//    pinMode(btn_stop            .pin, INPUT);

    pinMode(PIN_POT_TCA_TEMPS_FERMETURE_AUTO      , INPUT);
    pinMode(PIN_POT_TLAV_TEMPS_TRAVAIL            , INPUT);
    pinMode(PIN_POT_TSFAS_TEMPS_DECALAGE_FERMETURE, INPUT);


}


// Mise a jour des E/S
void update_inputs_and_ouputs(void)
{
    int analogValue; 
   
    btn_FDC_open[M1]     .loop();
    btn_FDC_open[M2]     .loop();
    btn_FDC_close[M1]    .loop();
    btn_FDC_close[M2]    .loop();
    btn_ouverture_pieton.loop();
    btn_start           .loop();
    btn_stop            .loop();
  
  // Rescale to potentiometer's voltage (from 0V to 3.3V):     
    analogValue = analogRead(PIN_POT_TCA_TEMPS_FERMETURE_AUTO      ); temps_fermeture_auto_ms     = map(analogValue, 0, 4095, 0, 15000);
    analogValue = analogRead(PIN_POT_TLAV_TEMPS_TRAVAIL            ); temps_travail_ms            = map(analogValue, 0, 4095, 0, 15000);
    analogValue = analogRead(PIN_POT_TSFAS_TEMPS_DECALAGE_FERMETURE); temps_decalage_fermeture_ms = map(analogValue, 0, 4095, 0, 15000);

}

/* etat du portail*/
typedef enum
{
    STATE_CLOSED ,
    STATE_WAIT_OPENING ,    
    STATE_OPENING,
    STATE_OPENED , 
    STATE_WAIT_CLOSING,
    STATE_CLOSING
} gate_state__t;

static gate_state__t main_state[2] = {STATE_CLOSED, STATE_CLOSED}; 

/* condition de demarrage ouverture */
/* - Start pressed
/**/
bool is_OPENING_requested(void)
{    return btn_start.isPressed();
}

/* condition de demarrage ouverture */
/* - Start pressed*/
bool is_CLOSING_requested(void)
{    return btn_start.isPressed();
}
/* attente ouverture M1 terminée */
/**/

bool is_WAIT_OPENING_completed(uint8_t iMotor)
{ 
    unsigned long mytime;    
    mytime =  millis();

    return (mytime > (t0_state[iMotor] + temps_delai_ouverture_ms[iMotor]));  // décalage ouverture  de M2 de 2s
}

/* indique condition d'ouverture terminée */
/* - Si "Start ou Stop pressed" après 200ms*/
/* - Timeout atteint (temps_travail_ms)  */
/* - FDC ouverture*/
/*   t0_state*/
/*   temps_travail_ms           */
/*   temps_decalage_fermeture_ms*/
bool is_OPENING_completed(uint8_t iMotor)
{ 
    unsigned long mytime;    
    mytime =  millis();

    return ((mytime > (t0_state[iMotor] + temps_travail_ms )) || btn_FDC_open[iMotor].isPressed());
}

bool is_WAIT_CLOSING_completed(uint8_t iMotor)
{ 
    unsigned long mytime;    
    mytime =  millis();

    return (mytime > (t0_state[iMotor] + temps_delai_fermeture_ms[iMotor] + (iMotor?temps_decalage_fermeture_ms:0)));  // décalage decalage fermeture variable
}

/* indique condition de fermeture terminée */
/* - Si "Start ou Stop pressed" après 200ms*/
/* - Timeout atteint (temps_travail_ms)*/
/* - FDC fermeture*/
bool is_CLOSING_completed(uint8_t iMotor)
{ 
    unsigned long mytime;    
    mytime =  millis();

    return ((mytime > (t0_state[iMotor] + temps_travail_ms )) || btn_FDC_close[iMotor].isPressed());
}
   

/* etat initial : tous les relays à OFF */
void gestion_state_stopped(void)
{
    digitalWrite(led_K1_TBD         , turnOFF);
    digitalWrite(led_K2_TBD         , turnOFF);
    digitalWrite(led_K3_sortie_lampe, turnOFF);
    digitalWrite(led_K4_ouverture_M1, turnOFF);
    digitalWrite(led_K5_ouverture_M2, turnOFF);
    digitalWrite(led_K6_fermeture_M1, turnOFF);
    digitalWrite(led_K7_fermeture_M2, turnOFF);
}
void gestion_state_opening(void)
{
    digitalWrite(led_K1_TBD         , turnOFF);
    digitalWrite(led_K2_TBD         , turnOFF);
    digitalWrite(led_K3_sortie_lampe, turnOFF);
    digitalWrite(led_K4_ouverture_M1, turnOFF);
    digitalWrite(led_K5_ouverture_M2, turnOFF);
    digitalWrite(led_K6_fermeture_M1, turnOFF);
    digitalWrite(led_K7_fermeture_M2, turnOFF);  
}
void gestion_state_opened(void)
{
    digitalWrite(led_K1_TBD         , turnOFF);
    digitalWrite(led_K2_TBD         , turnOFF);
    digitalWrite(led_K3_sortie_lampe, turnOFF);
    digitalWrite(led_K4_ouverture_M1, turnOFF);
    digitalWrite(led_K5_ouverture_M2, turnOFF);
    digitalWrite(led_K6_fermeture_M1, turnOFF);
    digitalWrite(led_K7_fermeture_M2, turnOFF);   
}
void gestion_state_closing(void)
{
    digitalWrite(led_K1_TBD         , turnOFF);
    digitalWrite(led_K2_TBD         , turnOFF);
    digitalWrite(led_K3_sortie_lampe, turnOFF);
    digitalWrite(led_K4_ouverture_M1, turnOFF);
    digitalWrite(led_K5_ouverture_M2, turnOFF);
    digitalWrite(led_K6_fermeture_M1, turnOFF);
    digitalWrite(led_K7_fermeture_M2, turnOFF);    
}
/* changement d'état 
/*  reset du t0_state */
static void set_main_state(uint8_t iMotor,  gate_state__t newstate)
{
    main_state[iMotor] = newstate;                         
    t0_state[iMotor] = millis(); 
}
void gestion_Portail(void)
{
    update_inputs_and_ouputs(); 
    if((main_state[M1] == STATE_CLOSED) && (main_state[M2] == STATE_CLOSED) && is_OPENING_requested())
    {   // synchro démarrage
        set_main_state(M1, STATE_WAIT_OPENING);
        set_main_state(M2, STATE_WAIT_OPENING);
    }

    if((main_state[M1] == STATE_OPENED) && (main_state[M2] == STATE_OPENED) && is_CLOSING_requested())
    {   // synchro démarrage
        set_main_state(M1, STATE_WAIT_CLOSING);
        set_main_state(M2, STATE_WAIT_CLOSING);
    }
    for (uint8_t iMotor =0; iMotor <2; iMotor++)
    {
    switch (main_state[iMotor])
        {   case STATE_CLOSED : 
                // géré en synchro
                break;
            case STATE_WAIT_OPENING: 
                if (is_WAIT_OPENING_completed(iMotor))
                {   set_main_state(iMotor, STATE_OPENING);             
                }
                break;
            case STATE_OPENING: 
                if (is_OPENING_completed(iMotor))
                {   set_main_state(iMotor, STATE_OPENED);             
                }
                break;        
            case STATE_OPENED : 
                // géré en synchro
                break;
            case STATE_WAIT_CLOSING: 
                if (is_WAIT_CLOSING_completed(iMotor))
                {   set_main_state(iMotor, STATE_CLOSING);             
                }
                break;            
            case STATE_CLOSING: 
                if (is_CLOSING_completed(iMotor))
                {   set_main_state(iMotor, STATE_CLOSED);
                }
                break;        
            default: set_main_state(iMotor, STATE_CLOSED);
        }
    } // pour chaque iMotor
}