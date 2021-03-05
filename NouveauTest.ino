// inclus la librairie:
#include <LiquidCrystal.h>

int capteurPositionPin = A0;
int commandePin = 2;

// PID
const float TENSION_CONSIGNE = 3.5;
const float DELTA_TEMPS = 0.1; // en secondes
const float CONSTANTE_PROPORTIONNELLE = 1;
const float CONSTANTE_INTEGRALE = 1;
const float CONSTANTE_DERIVEE = 1;
const float TENSION_COMMANDE_MAX = 1.3;
const float TENSION_COMMANDE_MIN = 0.7;

float derniereTension = 0.0;
float sommeErreurs = 0.0;
int indiceUniteDeLaMasse = 0; // 0 si c'est en gramme et 1 si c'est en oz
int buttonsState = 0; // État des boutons live
int lastButtonState = 0; // État précédent des boutons
String messageLigneDuHaut = "Bienvenue!";
String messageLigneDuBas;
float masseDeQualibrage = 0.00;

// On intialise la librairie avec les pins utilisées
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// On définit la valeur des boutons et des clés utilisés
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
 if (adc_key_in < 50)   return btnRIGHT; 
 if (adc_key_in < 250)  return btnUP;
 if (adc_key_in < 450)  return btnDOWN;
 if (adc_key_in < 650)  return btnLEFT;
 if (adc_key_in < 850)  return btnSELECT; 

 return btnNONE;  // when all others fail, return this...
}

// Fonction pour changer de grammes à ounces
String uniteDeLaMasse(float masse) {
  String masseAvecUnites;
  if (indiceUniteDeLaMasse == 0){
    masseAvecUnites = String(masse) + " g";
    indiceUniteDeLaMasse = 1;
  }
  else{
    masseAvecUnites = String(masse/28.35) + " oz";
    indiceUniteDeLaMasse = 0;
  }
  return masseAvecUnites;
}

// Fonction pour identifier le type de pièces
String typeDePiece(float masse) {
  String typeDePiece;
  if ((0.97 < masse/3.95 and masse/3.95 < 1.03) or (0.97 < masse/4.6 and masse/4.6 < 1.03)) {
    typeDePiece = "1 x 0.05$";
  } else if ((0.97 < masse/1.75 and masse/1.75 < 1.03) or (0.97 < masse/2.07 and masse/2.07 < 1.03)){
    typeDePiece = "1 x 0.10$";
  } else if ((0.97 < masse/5.05 and masse/5.05 < 1.03) or (0.97 < masse/4.400 and masse/4.400 < 1.03)){
    typeDePiece = "1 x 0.25$";
  } else if ((0.97 < masse/6.27 and masse/6.27 < 1.03) or (0.97 < masse/7 and masse/7 < 1.03)){
    typeDePiece = "1 x 1.00$";
  } else if ((0.97 < masse/6.92 and masse/6.92 < 1.03) or (0.97 < masse/7.3 and masse/7.3 < 1.03)){
    typeDePiece = "1 x 2.00$";
  } else {
    typeDePiece = "Mauvaise pièce";
  }
  return typeDePiece;
}

// Fonction pour changer la masse tare de la balance
float masseTare (float masse) {
   masseDeQualibrage = masse;
   return masseDeQualibrage;
}

void lireEntrees(){ // Fonction pour lire les entrées
   buttonsState = analogRead(0);
}

void ecrireSorties(){ // Fonction pour écrire les sorties
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(messageLigneDuHaut);
  lcd.setCursor(0,1);
  lcd.print(messageLigneDuBas);
}

void setMasse(float masse){
   if (buttonsState != lastButtonState and buttonsState != lastButtonState + 1 and buttonsState != lastButtonState - 1) {
       if (buttonsState < 60) { // Quand on clique sur le bouton right
        messageLigneDuHaut = "Nouvelle masse tare:";
        messageLigneDuBas = masseTare(masse);
       }
       else if (buttonsState < 200) {
         lcd.print ("Up    ");
       }
       else if (buttonsState < 400){
         lcd.print ("Down  ");
       }
       else if (buttonsState < 600){ // Quand on clique sur le bouton left
         messageLigneDuHaut = "Authentification";
         messageLigneDuBas = typeDePiece(masse);
       }
       else if (buttonsState < 800){ // Quand on clique sur le bouton select
         messageLigneDuHaut = "Masse totale:";
         messageLigneDuBas = uniteDeLaMasse(masse);
       }
   }
   lastButtonState = buttonsState; // On sauvegarde l'état actuel comme étant l'état précédent
}

void afficherTensionPosition(float tensionPosition) {
  Serial.print("Tension de position: ");
  Serial.print(tensionPosition);
  Serial.println(" V");
}

void afficherCommande(float commande) {
  Serial.print("Commande: ");
  Serial.print(commande);
  Serial.println(" V");
}

float getTensionCommandePID(float tensionActuelle) {
  float erreur = TENSION_CONSIGNE - tensionActuelle;

  float termeProportionnel = CONSTANTE_PROPORTIONNELLE * erreur;
  float termeIntegrale = CONSTANTE_INTEGRALE * (sommeErreurs + erreur) * DELTA_TEMPS;
  float termeDerivee = CONSTANTE_DERIVEE * (tensionActuelle - derniereTension) / DELTA_TEMPS;

  float commande = termeProportionnel + termeIntegrale + termeDerivee;

  // mise a jour des variables
  derniereTension = tensionActuelle;
  sommeErreurs += erreur; // a valider

  // verification de securite
  if (commande > TENSION_COMMANDE_MAX) {
    // Serial.println("Attention! tension de commande maximale.");
    commande = TENSION_COMMANDE_MAX;
  } else if (commande < TENSION_COMMANDE_MIN) {
    // Serial.println("Attention! tension de commande minimale.");
    commande = TENSION_COMMANDE_MIN;
  }
  
  return commande;
}

void setup() {
  pinMode(commandePin, OUTPUT);
  
  Serial.begin(9600);

  lcd.begin(16, 2);
}

void loop() {
  float masse = 7.30;
  lireEntrees();
  setMasse(masse - masseDeQualibrage);
  ecrireSorties();
  
  int capteurPositionValue = analogRead(capteurPositionPin);
  float tensionPosition = (5.0 / 1024) * capteurPositionValue;
  float commandeTension = getTensionCommandePID(tensionPosition);
  int commandeTensionDiscret = (int) (255 / 5.0) * commandeTension;
  analogWrite(commandePin, commandeTensionDiscret);
  Serial.print("consigne:"); Serial.print(TENSION_CONSIGNE); Serial.print(" ");
  Serial.print("capteur:"); Serial.print(tensionPosition); Serial.print(" ");
  Serial.print("commande:"); Serial.print(commandeTension); Serial.print("\n");
  // Serial.println(commandeTensionDiscret);
  delay(1000 * DELTA_TEMPS);
}
