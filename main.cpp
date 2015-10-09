/*
*********************************************************************************************************
*              Example code for IoT HTTP client Haze Reader using mbed applications board.
*
*                                     Copyright (c) 2015 Timothy Teh
*
*                     Licensed under the Apache License, Version 2.0 (the "License");
*                    you may not use this file except in compliance with the License.
*                                 You may obtain a copy of the License at
*
*                               http://www.apache.org/licenses/LICENSE-2.0
*
*                   Unless required by applicable law or agreed to in writing, software
*                    distributed under the License is distributed on an "AS IS" BASIS,
*                WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*                   See the License for the specific language governing permissions and
*                                     limitations under the License.
*********************************************************************************************************
*/

#include "mbed.h"
#include "EthernetInterface.h"
#include "HTTPClient.h"
#include "C12832_lcd.h"
#include "slre.h"

C12832_LCD lcd;

EthernetInterface eth;
HTTPClient http;
char str[4096];
char chHighestPSI[3];

static  uint8_t  u8MinuteCounter = 4;       /* Check the RSS feed only every 5 minutes. */

struct slre  reTimeStamp;                   /* Search for the timestamp. */
struct cap   capTimeStamp[2];

struct slre  rePSIVal;                      /* Search for the PSI value. */
struct cap   capPSIVal[2];
 
struct slre  reHighestPSI;                  /* Search for the highest PSI value in the range. */
struct cap   capHighestPSI[2];

PwmOut pwmR (p23);
PwmOut pwmG (p24);
PwmOut pwmB (p25);

int main() 
{
    eth.init(); //Use DHCP
    
    eth.connect();

    while(1) {
    
       slre_compile(&reTimeStamp,"<pubDate>(.*?)</pubDate>");    
       slre_compile(&rePSIVal," <psi>(.*?)</psi>");
       slre_compile(&reHighestPSI,": .*-(.*?) (");
       
       u8MinuteCounter++;
       if (u8MinuteCounter == 5)
       {
            u8MinuteCounter = 0;
            //GET data
            printf("\nTrying to fetch page...\n");
            int ret = http.get("http://www.haze.gov.sg/data/rss/nea_psi.xml", str, 4000);
            if (!ret)
            {
                lcd.cls();
                lcd.locate(0,0);
                //lcd.printf("PSI: \n");      
                printf("Page fetched successfully - read %d characters\n", strlen(str));
                /*printf("Result: %s\n", str);*/
                if (slre_match(&reTimeStamp, str, strlen(str), capTimeStamp)) {
                    lcd.printf("%.*s\n", capTimeStamp[1].len, capTimeStamp[1].ptr);
                }
                
                if (slre_match(&rePSIVal, str, strlen(str), capPSIVal)) {
                    lcd.printf("%.*s\n", capPSIVal[1].len, capPSIVal[1].ptr);
                }
                
                if (slre_match(&reHighestPSI, capPSIVal[1].ptr, capPSIVal[1].len, capHighestPSI)) {
                    printf("Highest PSI = %.*s\n", capHighestPSI[1].len, capHighestPSI[1].ptr);
                    memset(chHighestPSI, ' ', sizeof(chHighestPSI));        //reset the buffer with whitespace
                    strncpy(chHighestPSI, capHighestPSI[1].ptr, capHighestPSI[1].len);                    
                }
                
                uint16_t u16HighestPSI = atoi(chHighestPSI);
                printf("Highest PSI converted = %d\n", u16HighestPSI);
                
                if (u16HighestPSI > 300) {
                    pwmR = 0.0;
                    pwmG = 1.0;
                    pwmB = 1.0;
                } else if (u16HighestPSI > 200) {
                    pwmR = 0.0;
                    pwmG = 0.4;
                    pwmB = 1.0;
                } else if (u16HighestPSI > 100) {
                    pwmR = 0.0;
                    pwmG = 0.0;
                    pwmB = 1.0;
                } else if (u16HighestPSI > 50) {
                    pwmR = 1.0;
                    pwmG = 0.56;
                    pwmB = 0.37;                
                } else {
                    pwmR = 0.87;
                    pwmG = 0.04;
                    pwmB = 0.94;                     
                }
            }
            else
            {
                printf("Error - ret = %d - HTTP return code = %d\n", ret, http.getHTTPResponseCode());
            }
        }    

        Thread::wait(60000);
    }
    eth.disconnect();    
}