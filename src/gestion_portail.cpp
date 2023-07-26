#include <Arduino.h>
#include "main_include.h"
#include <ezButton.h>
#include <ezLED.h>

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


// usage : 
// led.turnON();
// led.loop();
// led.toogle();
// led.turnOFF(unsigned long delayTime_ms)
// led.blink()


ezLED led_K1_TBD          = { PIN_K1_TBD, CTRL_CATHODE };
ezLED led_K2_TBD          = { PIN_K2_TBD, CTRL_CATHODE };
ezLED led_K3_sortie_lampe = { PIN_K3_SORTIE_LAMPE, CTRL_CATHODE };
ezLED led_K4_ouverture_M1 = { PIN_K4_OUVERTURE_M1, CTRL_CATHODE };
ezLED led_K5_ouverture_M2 = { PIN_K5_OUVERTURE_M2, CTRL_CATHODE };
ezLED led_K6_fermeture_M1 = { PIN_K6_FERMETURE_M1, CTRL_CATHODE };
ezLED led_K7_fermeture_M2 = { PIN_K7_FERMETURE_M2, CTRL_CATHODE };
ezButton btn_FDC_open_M1      = { PIN_IN_FCA1_OUVERTURE_M1 };
ezButton btn_FDC_open_M2      = { PIN_IN_FCA2_OUVERTURE_M2 };
ezButton btn_FDC_close_M1     = { PIN_IN_FCC1_FERMETURE_M1 };
ezButton btn_FDC_close_M2     = { PIN_IN_FCC2_FERMETURE_M2 };
ezButton btn_ouverture_pieton = { PIN_IN_OUVERTURE_PIETON };
ezButton btn_start            = { PIN_IN_START };
ezButton btn_stop             = { PIN_IN_STOP };


static int t0_state =0;  // pour timing ouverture/fermeture  : temps initial

uint16_t temps_fermeture_auto_ms    ; 
uint16_t temps_travail_ms           ; 
uint16_t temps_decalage_fermeture_ms; 


// setup des E/S 
void setup_inputs_and_ouputs(void)
{
    pinMode(led_K1_TBD         .pin, OUTPUT);
    pinMode(led_K2_TBD         .pin, OUTPUT);
    pinMode(led_K3_sortie_lampe.pin, OUTPUT);
    pinMode(led_K4_ouverture_M1.pin, OUTPUT);
    pinMode(led_K5_ouverture_M2.pin, OUTPUT);
    pinMode(led_K6_fermeture_M1.pin, OUTPUT);
    pinMode(led_K7_fermeture_M2.pin, OUTPUT);

    pinMode(btn_FDC_open_M1     .pin, INPUT);
    pinMode(btn_FDC_open_M2     .pin, INPUT);
    pinMode(btn_FDC_close_M1    .pin, INPUT);
    pinMode(btn_FDC_close_M2    .pin, INPUT);
    pinMode(btn_ouverture_pieton.pin, INPUT);
    pinMode(btn_start           .pin, INPUT);
    pinMode(btn_stop            .pin, INPUT);

    pinMode(PIN_POT_TCA_TEMPS_FERMETURE_AUTO      , INPUT);
    pinMode(PIN_POT_TLAV_TEMPS_TRAVAIL            , INPUT);
    pinMode(PIN_POT_TSFAS_TEMPS_DECALAGE_FERMETURE, INPUT);


}


// Mise a jour des E/S
void update_inputs_and_ouputs(void)
{
    int analogValue; 
    led_K1_TBD         .update();
    led_K2_TBD         .update();
    led_K3_sortie_lampe.update();
    led_K4_ouverture_M1.update();
    led_K5_ouverture_M2.update();
    led_K6_fermeture_M1.update();
    led_K7_fermeture_M2.update();
    
    btn_FDC_open_M1     .read();
    btn_FDC_open_M2     .read();
    btn_FDC_close_M1    .read();
    btn_FDC_close_M2    .read();
    btn_ouverture_pieton.read();
    btn_start           .read();
    btn_stop            .read();
  
  // Rescale to potentiometer's voltage (from 0V to 3.3V):     
    temps_fermeture_auto_ms     = analogRead(PIN_POT_TCA_TEMPS_FERMETURE_AUTO      ); temps_fermeture_auto_ms     = map(analogValue, 0, 4095, 0, 15000);
    temps_travail_ms            = analogRead(PIN_POT_TLAV_TEMPS_TRAVAIL            ); temps_travail_ms            = map(analogValue, 0, 4095, 0, 15000);
    temps_decalage_fermeture_ms = analogRead(PIN_POT_TSFAS_TEMPS_DECALAGE_FERMETURE); temps_decalage_fermeture_ms = map(analogValue, 0, 4095, 0, 15000);

}

/* etat du portail*/
typedef enum
{
    CLOSED ,
    OPENING,
    OPENED , 
    CLOSING
} gate_state__t;

static gate_state__t main_state = CLOSED; 

/* condition de demarrage ouverture */
/* - Start pressed
/**/
bool opening_condition(void)
{    return btn_start.isPressed();
}

/* condition d'ouverture terminée */
/* - Start ou Stop pressed 
/* - Timeout atteind
/* - FDC ouverture
/* */
bool opening_completed(void)
{ 
    temps_travail_ms           
    temps_decalage_fermeture_ms
    
        return btn_start.isPressed();
}


void gestion_Portail(void)
{
    update_inputs_and_ouputs(); 
    switch (main_state)
    {   case CLOSED : 
            if (is_opening_condition())
            { set_main_state(OPENING);             
            }
            break;
        case OPENING: 
            if (opening_completed())
            {   set_main_state(OPENED);             
            }
            break;        
        case OPENED : 
            if (is_closing_condition())
            {   set_main_state(CLOSING);             
            }
            break;
        case CLOSING: 
            if (closing_completed())
            {   set_main_state(CLOSED);
            }
            break;        
        default: set_main_state(CLOSED);

    }
}
/* changement d'état 
/* + reset du t0_state */
static void set_main_state(gate_state__t main_state)
{
    main_state = main_state;                         
    t0_state = millis(); 
}


    

/* etat initial : tous les relays à OFF */
void gestion_state_stopped(void)
{
    led_K1_TBD         .on = 0;
    led_K2_TBD         .on = 0;
    led_K3_sortie_lampe.on = 0;
    led_K4_ouverture_M1.on = 0;
    led_K5_ouverture_M2.on = 0;
    led_K6_fermeture_M1.on = 0;
    led_K7_fermeture_M2.on = 0;    
}
void gestion_state_opening(void)
{
    led_K1_TBD         .on = 0;
    led_K2_TBD         .on = 0;
    led_K3_sortie_lampe.on = 0;
    led_K4_ouverture_M1.on = 0;
    led_K5_ouverture_M2.on = 0;
    led_K6_fermeture_M1.on = 0;
    led_K7_fermeture_M2.on = 0;    
    
}
void gestion_state_opened(void)
{
    led_K1_TBD         .on = 0;
    led_K2_TBD         .on = 0;
    led_K3_sortie_lampe.on = 0;
    led_K4_ouverture_M1.on = 0;
    led_K5_ouverture_M2.on = 0;
    led_K6_fermeture_M1.on = 0;
    led_K7_fermeture_M2.on = 0;    
    
}
void gestion_state_closing(void)
{
    led_K1_TBD         .on = 0;
    led_K2_TBD         .on = 0;
    led_K3_sortie_lampe.on = 0;
    led_K4_ouverture_M1.on = 0;
    led_K5_ouverture_M2.on = 0;
    led_K6_fermeture_M1.on = 0;
    led_K7_fermeture_M2.on = 0;    
    
}