#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h> // mixture of C and C++ :/
#include <string>
#include <iostream>
#include <sstream>  // stringstream
#include <unistd.h> // sleep() and read for temp sensor

#include <fcntl.h>  // for sensor
#include <dirent.h> // for sensor

#include <wiringPiI2C.h> // for LCD
#include <wiringPi.h>    // for LCD

/// ****************** LCD RELATED ********************

// Define some device parameters
#define I2C_ADDR 0x27 // I2C device address

// Define some device constants
#define LCD_CHR 1 // Mode - Sending data
#define LCD_CMD 0 // Mode - Sending command

// Define line start
#define LINE1 0x80 // 1st line
#define LINE2 0xC0
#define LINE3 0x94
#define LINE4 0xD4

#define LCD_BACKLIGHT 0x08 // On
// LCD_BACKLIGHT = 0x00  # Off

#define ENABLE 0b00000100 // Enable bit

void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);

// added by Lewis for LCD
void typeInt(int i);
void typeFloat(float myFloat);
void lcdLoc(int line); // move cursor
void ClrLcd(void);     // clr LCD return home
void typeln(const char *s);
void typeChar(char val);
int fd; // seen by all subroutines

// END OF LCD RELATED

// constants
int static sleepTime = 60; // time to sleep before updating stuff

// global variables
int outTemp = 0;
bool useSensor = false; // Use real temp sensor or random values
int debug = 1;          // RUN "EMULATION" MODE

// functions definitions

int temperature(void);               // temperature sensor
int temperature_time(void);          // get the time the heater should be on depending on temperature
int MinuteTime(void);                // get time in minutes since 00:00
void PrintArray(int TheArray[7][3]); // print pretty array of stored times
std::string ToTime(int minutetime);  // convert HH:MM to minutes
int WeekDay(void);                   // return day of week
void PowerStatus(int power);         // fake power function that would control heater
void ReadFile(void);                 // read TimeArray from file

// create empty global TimeArray
int TimeArray[7][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

int main()
{ // ************* MAIN FUNCTION **************

    // *********** LCD ************************
    if (wiringPiSetup() == -1)
        exit(1);

    fd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init(); // setup LCD

    ClrLcd();
    lcdLoc(LINE1);
    typeln("*** Heater  4000 ***");

    // END OF LCD RELATED

    /// ****************** VARIABLES ********************
    int i = 0, j = 1, k = 0; // j is minutes since midnight
    int d = 0, t = 0;
    int power = 0; // if power = 1 heater is on
    int day = 0;   // day of week
    int u = 0;
    char realtime[10];
    char pwrstat[5];
    char daystat[10];

    // weekday names
    char DayArray[7][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

    // read timearray from file
    ReadFile();

    // ****************** MAIN PROGRAM LOOP *****************

    while (1)
    {

        // check for "emulation" mode for testing (dont want to wait a week :)
        if (debug == 1)
        {
            j += 5; // increase time 5 minutes every loop
            usleep(100000);
            if (j > 1440)
            {
                j = 1;
                day++; // emulate new weekday if 1440 minutes = 24h has passed
            }
            if (day >= 7) // if week has "passed" restart week
                day = 0;
        }
        else
        { // If program is run in realtime use the values from functions
            j = MinuteTime();
            day = WeekDay();
        }

        // read the array from file every third pass of the loop (in realtime use about 5 minutes)
        if (k >= 5)
        {
            ReadFile();
            k = 0;
        }
        k++;

        // ****************** CONSOLE OUTPUT ************************

        if (power == 1) // power status in on/off text instead of 1/0.
            strcpy(pwrstat, "on");
        else
            strcpy(pwrstat, "off");

        printf("\n *************************************\n");
        printf("********** Block Heater 4000 **********\n");
        printf(" *************************************\n");
        printf("\n           Temp  :\t%d\n", temperature());
        printf("           Power :\t%s\n\n", pwrstat);
        // printf("           %s\t%s \n", DayArray[day], ToTime(j));
        std::cout << "           " << DayArray[day] << "\t" << ToTime(j) << "\n";
        printf("\n            CTRL+C to exit\n");
        printf("  ***********************************\n");

        // ****************** LCD ************************

        // The LCD code uses C so only char type of strings. These need to be converted...
        const char *timePrint = ToTime(j).c_str();
        std::string stringy = std::to_string(outTemp);
        const char *tempPrint = stringy.c_str();

        lcdLoc(LINE2);
        typeln(DayArray[day]);

        lcd_byte(0xCF, LCD_CMD);
        typeln(timePrint);

        lcd_byte(0xA3, LCD_CMD);
        typeln("    ");
        lcd_byte(0xA3, LCD_CMD);
        typeln(tempPrint);
        typeln("c");

        lcdLoc(LINE4);
        if (power == 1)
        {
            typeln("Power On");
        }
        typeln("        ");

        // ******** Power heater on/off logic *********

        for (t = 0; t < 3; t++)
        {
            // power on if "time - temperature() = current time (but using larger/smaller than in actual logic)
            if (TimeArray[day][t] - temperature_time() < j && TimeArray[day][t] > j && TimeArray[day][t] != 0 && power == 0)
            { //
                // printf("\n   POWER ON at %s due to slot %d\n", ToTime(j), t + 1);
                std::cout << "\n         POWER ON at " << ToTime(j) << " due to slot " << t + 1 << "\n";
                power = 1;
                PowerStatus(power); // this function controls actual heater
                if (debug == 1)     // to be able to see power on while running "emulation"
                    sleep(5);
            }
            // power off if time found in array
            if (TimeArray[day][t] < j && TimeArray[day][t] + 15 > j && TimeArray[day][t] != 0 && power == 1)
            {
                // printf("\n         POWER OFF at %s \n", ToTime(j));
                std::cout << "\n         POWER OFF at " << ToTime(j) << "\n";
                power = 0;
                PowerStatus(power); // this function controls actual heater
                if (debug == 1)     // to be able to see power off while running "emulation"
                    sleep(5);
            }
        }

        if (debug == 0)       // if not debugging/emulation wait 5 minutes
            sleep(sleepTime); // sleep for a while until updating everything

    } // end of main program loop

    return (0);
}

// *** MAIN PROGRAM END *****************

// ****************** FUNCTIONS ************************

// generate some random "temperature" value - in real life should be a sensor
int temperature(void)
{
    int sensor = 0;

    if (useSensor)
    {

        // This code below is from the internet for the sensor = not my code!
        DIR *dir;
        struct dirent *dirent;
        char dev[16];      // Dev ID
        char devPath[128]; // Path to device
        char buf[256];     // Data from device
        char tmpData[6];   // Temp C * 1000 reported by device
        char path[] = "/sys/bus/w1/devices";
        ssize_t numRead;

        dir = opendir(path);
        if (dir != NULL)
        {
            while ((dirent = readdir(dir)))
                // 1-wire devices are links beginning with 28-
                if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL)
                {
                    strcpy(dev, dirent->d_name);
                    // printf("\nDevice: %s\n", dev);
                }
            (void)closedir(dir);
        }
        else
        {
            perror("Couldn't open the w1 devices directory");
            return 1;
        }

        // Assemble path to OneWire device
        sprintf(devPath, "%s/%s/w1_slave", path, dev);
        // Read temp continuously
        // Opening the device's file triggers new reading

        int fd = open(devPath, O_RDONLY);
        if (fd == -1)
        {
            perror("Couldn't open the w1 device.");
            return 1;
        }
        while ((numRead = read(fd, buf, 256)) > 0)
        {
            strncpy(tmpData, strstr(buf, "t=") + 2, 5);
            float tempC = strtof(tmpData, NULL);
            // printf("Device: %s  - ", dev);
            // printf("Temp: %.3f C  ", tempC / 1000);
            // printf("%.3f F\n\n", (tempC / 1000) * 9 / 5 + 32);
            sensor = static_cast<int>(tempC / 1000); // casto to integer
        }
        close(fd);
    }

    else
    {
        srand(time(0));
        sensor = (rand() % 20 + 1) * -1;
    }
    outTemp = sensor; // update global sensor variable
    return (sensor);
}

// how long to keep heater on (time examples taken from some existing product from clas ohlson - could be more steps or a real function)
int temperature_time(void)
{
    int sensor = 0;
    sensor = temperature();

    if (sensor < -15)
        return (180);
    if (sensor < -10)
        return (120);
    if (sensor < -5)
        return (90);
    if (sensor < 0)
        return (60);
    if (sensor < 5)
        return (30);
    if (sensor > 5)
        return (-1);
    return 90; // if no temperature data return 90 as default
}

// print time array
void PrintArray(int TheArray[7][3])
{
    int i, j;
    // static weekday names
    char DayArray[7][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

    printf("\n");
    printf("Weekday: \tSlot 1|\tSlot 2|\tSlot 3|\n"); // explanations

    for (i = 0; i < 7; i++)
    {
        printf("(%d) %s  \t", i + 1, DayArray[i]); // print weekdays
        for (j = 0; j < 3; j++)
            if (TheArray[i][j] == 0)
                printf("  - \t"); // if time is 00:00 print '-'
            else
                std::cout << ToTime(TheArray[i][j]); // print time in 'human format'

        printf("\n");
    }
}

// function converts minutetime to human readable time
std::string ToTime(int minutetime)
{

    std::string buffer;
    std::string hour_s;
    std::string minutes_s;
    std::ostringstream out;

    int hour = minutetime / 60;
    int minutes = minutetime - hour * 60;

    if (hour <= 9)
        out << "0" << hour;
    else
        out << hour;

    out << ":";

    if (minutes <= 9)
        out << "0" << minutes; // adds zero if value under 10. looks better.
    else
        out << minutes;

    // buffer = std::to_string(minutetime);
    // std::cout << buffer << "--";
    //  buffer = "13:30";
    buffer = out.str();

    return buffer;
}

// convert time of day to minutes since 00:00 (minutetime)
// Probably are easier ways to get minutes. I just made this and it did work. So im happy. :)
int MinuteTime(void)
{
    int minutetime = 0;

    time_t rawtime;
    struct tm *timeinfo;
    char thour[5];
    char tminute[5];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(thour, 80, "%H", timeinfo);           // get hours
    strftime(tminute, 80, "%M", timeinfo);         // get minutes
    minutetime = atoi(thour) * 60 + atoi(tminute); // convert to integers and add hours * 60 + minutes

    return (minutetime);
}

// return day of week
int WeekDay(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    int weekday = 0;
    char tday[5];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(tday, 80, "%w", timeinfo); // get day of week (0 is sunday)
    weekday = atoi(tday) - 1;           // -1 to get monday as 0
    if (weekday == -1)                  // make sure sunday is 6
        weekday = 6;
    return (weekday);
}

// activate heater.
void PowerStatus(int power)
{
    // mqtt commands
    std::string onLidl1 = "mosquitto_pub -d -t zigbee2mqtt/lidl1/set -m '{\"state\": \"ON\"}' >null";
    std::string offLidl1 = "mosquitto_pub -d -t zigbee2mqtt/lidl1/set -m '{\"state\": \"OFF\"}' >null";

    if (power == 1)
    {
        /* power on code */

        system(onLidl1.c_str());
    }
    if (power == 0)
    {
        /* power off code */

        system(offLidl1.c_str());
    }
}

void ReadFile(void)
{
    // read from file
    FILE *fp = NULL;
    fp = fopen("timearray.txt", "r");
    if (fp == NULL)
    {
        printf("\n** Error opening file for reading! Creating new file **\n");
        fp = fopen("timearray.txt", "w");
    }
    else
    {
        // fread to read binary file
        // fread(TimeArray, sizeof(int), 7*3, fp);

        // this version reads the data as text.
        char read_buffer[10];
        int i, j;
        for (i = 0; i < 7; i++)
        {
            for (j = 0; j < 3; j++)
            {
                fgets(read_buffer, 60, fp);
                TimeArray[i][j] = atoi(read_buffer);
            }
        }
    }
    fclose(fp);
}

// LCD FUNCTIONS ///////////////////

// int to string
void typeInt(int i)
{
    char array1[20];
    sprintf(array1, "%d", i);
    typeln(array1);
}

// clr lcd go home loc 0x80
void ClrLcd(void)
{
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}

// go to location on LCD
void lcdLoc(int line)
{
    lcd_byte(line, LCD_CMD);
}

// out char to LCD at current position
void typeChar(char val)
{

    lcd_byte(val, LCD_CHR);
}

// this allows use of any size string
void typeln(const char *s)
{

    while (*s)
        lcd_byte(*(s++), LCD_CHR);
}

void lcd_byte(int bits, int mode)
{

    // Send byte to data pins
    //  bits = the data
    //  mode = 1 for data, 0 for command
    int bits_high;
    int bits_low;
    // uses the two half byte writes to LCD
    bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    // High bits
    wiringPiI2CReadReg8(fd, bits_high);
    lcd_toggle_enable(bits_high);

    // Low bits
    wiringPiI2CReadReg8(fd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)
{
    // Toggle enable pin on LCD display
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
    delayMicroseconds(500);
}

void lcd_init()
{
    // Initialise display
    lcd_byte(0x33, LCD_CMD); // Initialise
    lcd_byte(0x32, LCD_CMD); // Initialise
    lcd_byte(0x06, LCD_CMD); // Cursor move direction
    lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
    lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
    lcd_byte(0x01, LCD_CMD); // Clear display
    delayMicroseconds(500);
}
// END OF LCD FUNCTIONS
