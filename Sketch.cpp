#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>
#include <LiquidCrystal.h>

void printSensorAdress(DeviceAddress deviceAddress);
void setup(void);
void Timer1ms(void);

int getButton();

void ManualTargetOffset(void);
void ManualPercentOffset(void);
void Monitoring(void);

#define ONE_WIRE_BUS 3

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

DeviceAddress sensorAdress;

LiquidCrystal display(8, 9, 4, 5, 6, 7);

#define BUTTON_RIGHT  0
#define BUTTON_UP     1
#define BUTTON_DOWN   2
#define BUTTON_LEFT   3
#define BUTTON_SELECT 4
#define BUTTON_NONE   5

#define OUTPUT_PIN LED_BUILTIN

bool doUpdate = 0;

float oldTemperature = 20;
float newTemperature = 0;

int analogInput  = 0;
int selectedMenu = 0;
int outputPercentage = 0;
int setTemperature = 20;
int outputLevel = LOW;
int runTime = 0;
int tick = 0;

unsigned int sensorCount;

byte degree[8] = { B00001100,
				   B00010010,
				   B00010010,
				   B00001100,
				   B00000000,
				   B00000000,
				   B00000000,
				   B00000000 };

void printSensorAdress(DeviceAddress deviceAddress) {
	for (uint8_t i = 0; i < 8; i++)  {
		if (deviceAddress[i] < 16) {
			Serial.print("0");
		}
		Serial.print(deviceAddress[i], HEX);
	}
}

void setup(void) {
	display.begin(16, 2);
	display.setCursor(0, 0);
	display.print("Cont. do Cowboy");

	Serial.begin(9600);

	sensors.begin();

	sensorCount = sensors.getDeviceCount();

	Serial.println("Scanning for DS18B20 sensors...");
	Serial.print(sensorCount, DEC);
	Serial.println(" sensors were found.");

	if (sensorCount) {
		if (!sensors.getAddress(sensorAdress, 0)) {
			Serial.println("No sensors found.");
		}
		Serial.print("Adress "); // Mostra o endereco do sensor encontrado no barramento
		printSensorAdress(sensorAdress);
		Serial.println();
		Serial.println();
	}

	pinMode(OUTPUT_PIN, OUTPUT);

	Timer1.initialize(1000);
	Timer1.attachInterrupt(Timer1ms);

	display.createChar(0, degree);

	Monitoring();
}

void Timer1ms(void) {
	static unsigned char Tempo10ms = 10;
	static unsigned char Tempo100ms = 100;
	static unsigned char powerOnTime = 0;

	if (!--Tempo10ms) {
		Tempo10ms = 10;

		if (powerOnTime) {
			if (!--powerOnTime) {
				digitalWrite(OUTPUT_PIN, false);
			}
		}

		if (!--Tempo100ms) {
			Tempo100ms = 100;

			doUpdate = true;

			if (outputPercentage) {
				digitalWrite(OUTPUT_PIN, true);
				powerOnTime = outputPercentage;
			}
		}
	}
}

void printWelcome() {
	display.setCursor(0, 0);
	display.print("Menus do Cowboy ");
}

void printMenu() {
	switch (selectedMenu) {
		case 0:
			display.setCursor(0, 1);
			display.print("Selec. ");
			display.write(byte(0));
			display.print("C max.  ");
			break;
		case 1:
			display.setCursor(0, 1);
			display.print("Selec. % saida  ");
			break;
		case 2:
			display.setCursor(0, 1);
			display.print("Monitoramento   ");
			break;
		default:
			display.setCursor(0, 1);
			display.print("                ");
			break;
	}
}

void printEmpty(int line) {
	display.setCursor(0, line);
	display.print("                ");
}

void printEmpty(int x1, int x2, int y) {
	while (x1 < x2) {
		display.setCursor(x1, y);
		display.print(" ");
		++x1;
	}
}

void incrementTick() {
	++tick;
}

float getTemperature() {
	newTemperature = sensors.getTempC(sensorAdress);
	if (newTemperature < 0) {
		--oldTemperature;
	} else if (newTemperature - oldTemperature > 1) {
		++oldTemperature;
	} else {
		oldTemperature = newTemperature;
	}
	return oldTemperature;
}

void resetTick() {
	tick = 0;
}

int getButton() {
	analogInput = analogRead(0);

	if (analogInput > 1000) return BUTTON_NONE;
	if (analogInput < 50)   return BUTTON_RIGHT;
	if (analogInput < 250)  return BUTTON_UP;
	if (analogInput < 450)  return BUTTON_DOWN;
	if (analogInput < 650)  return BUTTON_LEFT;
	if (analogInput < 850)  return BUTTON_SELECT;

	return BUTTON_NONE;
}

void log() {
	Serial.print(runTime);
	Serial.print(' ');
	Serial.print((int) (getTemperature() * 10));
	Serial.print('\n');
}

void loop() {
	static int button = BUTTON_NONE;
	static int previousButton = BUTTON_NONE;

	if (getButton() != previousButton) {
		previousButton = getButton();
		switch (getButton()) {
			case BUTTON_SELECT:
				switch (selectedMenu) {
					case 0:
						ManualTargetOffset();
						break;
					case 1:
						ManualPercentOffset();
						break;
					case 2:
						Monitoring();
					default:
						break;
					}
			case BUTTON_UP:
				selectedMenu + 1 > 2 ? selectedMenu = 0 : ++selectedMenu;
				printMenu();
				break;
			case BUTTON_DOWN:
				selectedMenu - 1 < 0 ? selectedMenu = 2: --selectedMenu;
				printMenu();
				break;
			default:
				break;
		}
	}


	if (sensorCount && doUpdate) {
		doUpdate = 0;

		sensors.requestTemperatures();

		float rawTemperature = getTemperature();

		if ((int) rawTemperature < setTemperature) {
			outputPercentage = (int) ((((float) setTemperature - rawTemperature) / (float) setTemperature) * 100);
		} else {
			digitalWrite(OUTPUT_PIN, false);
		}
	}
}

void Monitoring(void) {
	static int key = BUTTON_NONE;

	resetTick();

	printEmpty(0);
	printEmpty(1);

	display.setCursor(0, 0);
	display.print("Temp.:          ");

	display.setCursor(0, 1);
	display.print("Max:            ");
	display.print("Aquecendo:      ");

	while (true) {
		key = getButton();

		if (tick > 2880) {
			resetTick();
			switch (key) {
				case BUTTON_SELECT:
					printWelcome();
					printMenu();
					return;
				default:
					break;
			}

			sensors.requestTemperatures();

			float rawTemperature = getTemperature();

			printEmpty(7, 16, 0);
			printEmpty(5, 16, 1);

			display.setCursor(7, 0);
			display.print(rawTemperature);
			display.write(byte(0));
			display.print("C");

			display.setCursor(5, 1);
			display.print(setTemperature);
			display.write(byte(0));
			display.print("C");

			display.setCursor(12, 1);
			display.print((int) ((((float) setTemperature - rawTemperature) / (float) setTemperature) * 100));
			display.print("%");

			++runTime;

			log();
		} else {
			incrementTick();
		}
	}
}

void ManualTargetOffset(void) {
	static int key = BUTTON_NONE;

	resetTick();

	printEmpty(0);
	printEmpty(1);

	display.setCursor(0, 0);
	display.print("Ajuste ");
	display.write(byte(0));
	display.print("C maximo");

	while (true) {
		key = getButton();

		if (tick > 1440) {
			resetTick();
			switch (key) {
				case BUTTON_UP:
					setTemperature = setTemperature + 1 < 999 ? setTemperature + 1 : 999;
					break;
				case BUTTON_DOWN:
					setTemperature = setTemperature - 1 > 0 ? setTemperature - 1 : -999;
					break;
				case BUTTON_SELECT:
					printWelcome();
					printMenu();
					return;
				default:
					break;
			}

			printEmpty(1);

			display.setCursor(0, 1);
			display.write(byte(0));
			display.print("C: ");
			display.print(setTemperature);
		} else {
			incrementTick();
		}
	}
}

void ManualPercentOffset(void) {
	static int key = BUTTON_NONE;

	resetTick();

	printEmpty(0);
	printEmpty(1);

	display.setCursor(0, 0);
	display.print("Ajuste % saida");

	display.setCursor(0, 1);
	display.print("%: ");

	while (true) {
		key = getButton();

		if (tick > 1440) {
			resetTick();
			switch (key) {
				case BUTTON_UP:
					if (outputPercentage < 100) {
						++outputPercentage;
					}
					break;
				case BUTTON_DOWN:
					if (outputPercentage) {
						--outputPercentage;
					}
					break;
				case BUTTON_SELECT:
					printWelcome();
					printMenu();
					return;
				default:
					break;
			}

			printEmpty(1);

			display.setCursor(0, 1);
			display.print("%: ");
			display.print(outputPercentage);
			display.print("%");
		} else {
			incrementTick();
		}
	}
}