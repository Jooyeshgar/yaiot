#include <EEPROM.h>

void save_ssid_to_eeprom(String string){
  EEPROM.write(255, string.length());
  for(int i = 0; i < (string.length()); i++){
      EEPROM.write(i, string.charAt(i));
      EEPROM.commit();
  }
  
}

void save_password_to_eeprom(String string){
  EEPROM.write(254, string.length());
  for(int i = 0; i < (string.length()); i++){
      EEPROM.write(i+100, string.charAt(i));
      EEPROM.commit();
  }
  
}

void read_ssid_from_eeprom(){
  int len = (int)EEPROM.read(255);
  char eeprom_buffer[100];
  for(int i = 0; i < (len); i++){
    eeprom_buffer[i] = EEPROM.read(i);
  }
  eeprom_buffer[len] = '\0';
  
  memcpy(ssid,eeprom_buffer, 100);
  Serial.println("in read function ssid is:");
  Serial.println(ssid);
}

void read_password_from_eeprom(){
  int len = (int)EEPROM.read(254);
  char eeprom_buffer[100];
  for(int i = 0; i < (len); i++){
    eeprom_buffer[i] = EEPROM.read(i+100);
  }
  eeprom_buffer[len] = '\0';
  memcpy(password, eeprom_buffer, 100);
  Serial.println("in read function password is: ");
  Serial.println(password);
}
