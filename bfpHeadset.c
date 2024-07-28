/*
          bfpHeadset
  (Bluetooth-FM-Phone Headset)
           Oct 2018
        By M. Yousefi
        www.dihav.com
    vahid_you2004@yahoo.com
*/

// default bluetooth pin code is 9852, you can change it at lines 281-284
#define CanHaveSIMCard    1   // comment for no SIM support

// pins
#define S800RX   GP0_bit
#define S800Pow  GP1_bit
#define RotPush  GP2_bit
#define RotClk   GP4_bit
#define RotDat   GP5_bit

// constants
#define ACT_BTON          95
#define ACT_FMON          105
#define ACT_NONE          100
#define ACT_DOWN_INC      0
#define ACT_DOWN_DEC      1
#define ACT_INC           2
#define ACT_DEC           3
#define FM_VOLUME_ADDR    10
#define FM_FREQ_ADDR      11
#define RECONFIG_ADDR     12
#define RECONFIG_VAL      192 // just a number from 0 to 254
#define BT_PAIRMODE_ADDR  13
#define RINGTONE_ADDR     14
#define RINGERLEVEL_ADDR  15
#define MICGAIN_ADDR      16
#define SPKVOL_ADDR       17
#define EEPROM_DUMMY_ADDR 250
#define NO_ACTIVITY_TIME  32  // seconds * 15.26 + 1
#define UART_DELAY        833 // 1000000 / baudrate(1200)
#define CONFMOD_FIRSTITEM 128
#define CONFMOD_BTPAIRING 128  
#define CONFMOD_SPKVOL    129
#define CONFMOD_MICGAIN   130
#define CONFMOD_RINGTONE  131
#define CONFMOD_RINGVOL   132
#define CONFMOD_LASTITEM  132

// AT commands
#define AT_AT            0 // test communication
#define AT_ATA           1 // answer incomming call
#define AT_ATH           2 // reject incomming call 
#define AT_Save          3 // save settings
#define AT_IPR          11 // sets the baudrate to 1200
#define AT_CFUN         12 // SIM functionallity
#define AT_CMGD_ALL     13 // delete all sms
#define AT_CNMI         14 // do not save new sms into memory
#define AT_PlayRingTone 15 // plays the ringtone
#define AT_SetRingTone  16 // changes the ringtone
#define AT_CRSL         17 // sets the ringer sound level
#define AT_CMIC         18 // sets the mic gain
#define AT_CLVL         19 // sets the speaker volume
#define AT_BTPOWEROFF   30 // turn on bluetooth
#define AT_BTPOWERON    31 // turn off bluetooth
#define AT_BTName       32 // set bluetooth name
#define AT_BTVIS        33 // make blutooth visible
#define AT_BTUNPAIR     34 // delete all paired devices
#define AT_BTPAIRCFG    35 // pairing config
#define AT_BTATA        36 // answer bluetooth incomming call
#define AT_BTATH        37 // reject bluetooth incomming call
#define AT_BTStop       40 // BTAVRCOP=0
#define AT_BTPlay       41 // BTAVRCOP=1
#define AT_BTPause      42 // BTAVRCOP=2
#define AT_BTNext       43 // BTAVRCOP=3
#define AT_BTPrevious   44 // BTAVRCOP=4
#define AT_BTVolumeUp   45 // BTAVRCOP=5
#define AT_BTVolumeDown 46 // BTAVRCOP=6
#define AT_FMOPEN       60 // turn on FM radio
#define AT_FMCLOSE      61 // turn off FM radio
#define AT_FMVOLUME     62 // change FM radio sound volume
#define AT_FMFREQ       63 // change FM radio freuency
#define AT_Beep         70 // make 50+tim*100ms 2500Hz beep  (60-69)

// variables
volatile bit Power, PushedNoRotate, LastClk, LastDat, RotHasPushed;
volatile char RotateAct, NoActivity, Seq, ConfigMode;
bit BTMode, BTIsPlaying, FMFreqChangeMode, LastPush;
char FMVolume, FMFreq;
char BTPairMode, RingTone, RingerLevel, MicGain, SpeakerVolume, BeepTim;

void UARTWrite(char dt) {
  char k;
  S800RX = 0b0;
  Delay_us(UART_DELAY);
  for (k = 1; k; k <<= 1) {
    if (dt & k) S800RX = 0b1; else S800RX = 0b0;
    Delay_us(UART_DELAY);
  }
  S800RX = 0b1;
  Delay_us(UART_DELAY);
}

void SendAT(char cmd, char wait200) {
  char k;
  GPIE_bit = 0b0;
  UARTWrite('A');
  UARTWrite('T');
  if (cmd > 9) UARTWrite('+');
#ifdef CanHaveSIMCard
  if (AT_ATA == cmd) UARTWrite('A');
  if (AT_ATH == cmd) UARTWrite('H');
#endif       
  if (AT_Save == cmd) {
    UARTWrite('&');
    UARTWrite('W');
    UARTWrite(13);
  }     
  if (AT_IPR == cmd) {
    UARTWrite('I');
    UARTWrite('P');
    UARTWrite('R');
    UARTWrite('=');
    UARTWrite('1');
    UARTWrite('2');
    UARTWrite('0');
    UARTWrite('0');
  }
  if ((AT_CFUN <= cmd) && (AT_CLVL >= cmd)) UARTWrite('C');
  if (AT_CFUN == cmd) {
    //UARTWrite('C');
    UARTWrite('F');
    UARTWrite('U');
    UARTWrite('N');
    UARTWrite('=');
#ifdef CanHaveSIMCard
    UARTWrite('1');
#else
    UARTWrite('0');
#endif
  }
#ifdef CanHaveSIMCard
  if (AT_CMGD_ALL == cmd) {
    //UARTWrite('C');
    UARTWrite('M');
    UARTWrite('G');
    UARTWrite('D');
    UARTWrite('=');
    UARTWrite('1');
    UARTWrite(',');
    UARTWrite('4');
  }
  if (AT_CNMI == cmd) {
    //UARTWrite('C');
    UARTWrite('N');
    UARTWrite('M');
    UARTWrite('I');
    UARTWrite('=');
    UARTWrite('2');
    UARTWrite(',');
    UARTWrite('2');
  }
  if ((AT_PlayRingTone == cmd) || (AT_SetRingTone == cmd)) {
    //UARTWrite('C');
    UARTWrite('A');
    UARTWrite('L');
    UARTWrite('S');
    UARTWrite('=');
    if (RingTone < 10)
      UARTWrite('0' + RingTone);
    else {
      UARTWrite('1');
      UARTWrite('0' + RingTone - 10);
    }
    if (AT_PlayRingTone == cmd) {
      UARTWrite(',');
      UARTWrite('1');
    }
  }
  if ((AT_CRSL == cmd) || (AT_CLVL == cmd)) {
    //UARTWrite('C');
    if (AT_CRSL == cmd) {
      UARTWrite('R');
      UARTWrite('S');
    } else {
      UARTWrite('L');
      UARTWrite('V');
    }
    UARTWrite('L');
    UARTWrite('=');
    if (BeepTim) UARTWrite(BeepTim + '0');
    UARTWrite('5');
  }
#endif
  if (AT_CMIC == cmd) {
    //UARTWrite('C');
    UARTWrite('M');
    UARTWrite('I');
    UARTWrite('C');
    UARTWrite('=');
    UARTWrite('0');
    UARTWrite(',');
    if (MicGain > 3) {
      UARTWrite('1');
      UARTWrite(MicGain - 4 + '0');
    } else
      UARTWrite(MicGain + 6 + '0');
  }
  if ((AT_BTPOWEROFF <= cmd) && (AT_BTVolumeDown >= cmd)) {
    UARTWrite('B');
    UARTWrite('T');
  }
  if ((AT_BTPOWERON == cmd) || (AT_BTPOWEROFF == cmd)) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('P');
    UARTWrite('O');
    UARTWrite('W');
    UARTWrite('E');
    UARTWrite('R');
    UARTWrite('=');
    UARTWrite('0' + cmd - AT_BTPOWEROFF);
  }
  if (AT_BTName == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('H');
    UARTWrite('O');
    UARTWrite('S');
    UARTWrite('T');
    UARTWrite('=');
    UARTWrite('d');
    UARTWrite('i');
    UARTWrite('h');
    UARTWrite('a');
    UARTWrite('v');
    UARTWrite(' ');
    UARTWrite('H');
    UARTWrite('e');
    UARTWrite('a');
    UARTWrite('d');
    UARTWrite('s');
    UARTWrite('e');
    UARTWrite('t');
  } 
  if (AT_BTVIS == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('V');
    UARTWrite('I');
    UARTWrite('S');
    UARTWrite('=');
    UARTWrite('1');
  }
  if (AT_BTUNPAIR == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('U');
    UARTWrite('N');
    UARTWrite('P');
    UARTWrite('A');
    UARTWrite('I');
    UARTWrite('R');
    UARTWrite('=');
    UARTWrite('0');
  }
  if (AT_BTPAIRCFG == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('P');
    UARTWrite('A');
    UARTWrite('I');
    UARTWrite('R');
    UARTWrite('C');
    UARTWrite('F');
    UARTWrite('G');
    UARTWrite('=');
    if (EEPROM_Read(BT_PAIRMODE_ADDR)) 
      UARTWrite('2');
    else {
      UARTWrite('1');
      UARTWrite(',');
      UARTWrite('9');
      UARTWrite('8');
      UARTWrite('5');
      UARTWrite('2');
    }
  }
  if ((AT_BTStop <= cmd) && (AT_BTVolumeDown >= cmd)) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('A');
    UARTWrite('V');
    UARTWrite('R');
    UARTWrite('C');
    UARTWrite('O');
    UARTWrite('P');
    UARTWrite('=');
    UARTWrite('0' + cmd - AT_BTStop);
  }
  if (AT_BTATA == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('A');
    UARTWrite('T');
    UARTWrite('A');
  }
  if (AT_BTATH == cmd) {
    //UARTWrite('B');
    //UARTWrite('T');
    UARTWrite('A');
    UARTWrite('T');
    UARTWrite('H');
  }
  if ((AT_FMOPEN <= cmd) && (AT_FMFREQ >= cmd)) {
    UARTWrite('F');
    UARTWrite('M');
  }
  if (AT_FMOPEN == cmd) {
    //UARTWrite('F');
    //UARTWrite('M');
    UARTWrite('O');
    UARTWrite('P');
    UARTWrite('E');
    UARTWrite('N');
    UARTWrite('=');
    UARTWrite('0');
  }
  if (AT_FMCLOSE == cmd) {
    //UARTWrite('F');
    //UARTWrite('M');
    UARTWrite('C');
    UARTWrite('L');
    UARTWrite('O');
    UARTWrite('S');
    UARTWrite('E');
  }
  if (AT_FMVOLUME == cmd) {
    //UARTWrite('F');
    //UARTWrite('M');
    UARTWrite('V');
    UARTWrite('O');
    UARTWrite('L');
    UARTWrite('U');
    UARTWrite('M');
    UARTWrite('E');
    UARTWrite('=');
    UARTWrite('0' + FMVolume);
  }
  if (AT_FMFREQ == cmd) {
    //UARTWrite('F');
    //UARTWrite('M');
    UARTWrite('F');
    UARTWrite('R');
    UARTWrite('E');
    UARTWrite('Q');
    UARTWrite('=');
    k = FMFreq;
    if (FMFreq < 25) {
      UARTWrite('8');
      k += 75;
    } else if (FMFreq < 125) {
      UARTWrite('9');
      k -= 25;
    } else {
      UARTWrite('1');
      UARTWrite('0');
      k -= 125;
    }
    UARTWrite('0' + k / 10);
    UARTWrite('0' + k % 10);
  }
  if (AT_Beep == cmd) {
    UARTWrite('S');
    UARTWrite('I');
    UARTWrite('M');
    UARTWrite('T');
    UARTWrite('O');
    UARTWrite('N');
    UARTWrite('E');
    UARTWrite('=');
    UARTWrite('1');
    UARTWrite(',');
    UARTWrite('2');
    UARTWrite('5');
    UARTWrite('0');
    UARTWrite('0');
    UARTWrite(',');
    UARTWrite('2');
    UARTWrite('0');
    UARTWrite('0');
    UARTWrite(',');
    UARTWrite('0');
    UARTWrite(',');
    if (BeepTim) UARTWrite(BeepTim + '0');
    UARTWrite('5');
    UARTWrite('0');
  }
  UARTWrite(10);
  GPIE_bit = 0b1;
  if (wait200) delay_ms(500); else delay_ms(200);
}

void PresentConfigVal() {
  if (CONFMOD_BTPAIRING == ConfigMode) {
    BeepTim = 2;
    SendAT(AT_Beep, 0);
    if (!BTPairMode) SendAT(AT_Beep, 0);
  } 
  if (CONFMOD_SPKVOL == ConfigMode) {
    BeepTim = SpeakerVolume;
    SendAT(AT_CLVL, 0);
    SendAT(AT_Beep, 0);
  }
  if (CONFMOD_MICGAIN == ConfigMode) {
    BeepTim = MicGain;
    SendAT(AT_CMIC, 0);
    SendAT(AT_Beep, 0);
  }
  if (CONFMOD_RINGTONE == ConfigMode)
    SendAT(AT_PlayRingTone, 0);
  if (CONFMOD_RINGVOL == ConfigMode) {
    BeepTim = RingerLevel;
    SendAT(AT_CRSL, 0);
  }
}

void TurnOn() {
  GIE_bit = 0b0;
  Power = 0b1;
  S800Pow = 0b0;
  Delay_ms(2000);
  S800Pow = 0b1;
  SendAT(AT_AT, 1);
  SendAT(AT_AT, 1);
  SendAT(AT_AT, 1);
  SendAT(AT_CFUN, 0);
  BTMode = RotateAct < ACT_BTON;
  if (ConfigMode.B7) {
    ConfigMode = CONFMOD_FIRSTITEM;
    SendAT(AT_BTPOWEROFF, 0);
    BTPairMode = EEPROM_Read(BT_PAIRMODE_ADDR);
    if (BTPairMode > 1) BTPairMode = 1; 
    SpeakerVolume = EEPROM_Read(SPKVOL_ADDR);
    if (SpeakerVolume > 9) SpeakerVolume = 3;
    MicGain = EEPROM_Read(MICGAIN_ADDR);
    if (MicGain > 9) MicGain = 4;
    RingTone = EEPROM_Read(RINGTONE_ADDR);
    if ((RingTone == 0) || (RingTone > 19)) RingTone = 1;
    RingerLevel = EEPROM_Read(RINGERLEVEL_ADDR);
    if (RingerLevel > 9) RingerLevel = 2;
    Delay_ms(2000);
    PresentConfigVal();
  } else if (BTMode) {
    if (EEPROM_Read(RECONFIG_ADDR) != RECONFIG_VAL) {
      SendAT(AT_BTPOWEROFF, 0);
      Delay_ms(3000);
      SendAT(AT_IPR, 0);
      SendAT(AT_CMGD_ALL, 0);
      SendAT(AT_CNMI, 0);
      SendAT(AT_Save, 0);
      SendAT(AT_BTName, 0);  
      SendAT(AT_BTPAIRCFG, 0);
      SendAT(AT_BTPOWERON, 0);
      Delay_ms(1000);
      SendAT(AT_BTVIS, 0);
      SendAT(AT_BTUNPAIR, 0);
      EEPROM_Write(RECONFIG_ADDR, RECONFIG_VAL);
    } else
      SendAT(AT_BTPOWERON, 0);
    BTIsPlaying = 0b0;
  } else {
    SendAT(AT_BTPOWEROFF, 0);
    FMVolume = EEPROM_Read(FM_VOLUME_ADDR);
    asm NOP;
    asm NOP;
    FMFreq = EEPROM_Read(FM_FREQ_ADDR);
    if (FMVolume > 6) FMVolume = 3;
    if (FMFreq > 205) FMFreq = 0;
    SendAT(AT_FMVOLUME, 0);
    SendAT(AT_FMOPEN, 0);
    SendAT(AT_FMFREQ, 0);
    FMFreqChangeMode = 0b0;
  }
  RotateAct = ACT_NONE; 
  EEADR = EEPROM_DUMMY_ADDR;
  GIE_bit = 0b1;
}

void TurnOff() {
  char v, f;
  GIE_bit = 0b0;
  if (!ConfigMode.B7) {
    if (BTMode) {
      SendAT(AT_BTPOWEROFF, 0);
      Delay_ms(3000);
    } else {
      v = EEPROM_Read(FM_VOLUME_ADDR);
      asm NOP;
      asm NOP;
      f = EEPROM_Read(FM_FREQ_ADDR);
      asm NOP;
      asm NOP;
      SendAT(AT_FMCLOSE, 0);
      if (v != FMVolume) EEPROM_Write(FM_VOLUME_ADDR, FMVolume);
      asm NOP;
      asm NOP;
      if (f != FMFreq) EEPROM_Write(FM_FREQ_ADDR, FMFreq);
      asm NOP;
      asm NOP;
      EEADR = EEPROM_DUMMY_ADDR;
    }
  }
  S800Pow = 0b0;
  Delay_ms(2000);
  S800Pow = 0b1;  
  Power = 0b0;
  GIE_bit = 0b1;
}

void CheckCfgMdDec() {
  if (ConfigMode < 10)
    ConfigMode = 10;
  else if ((ConfigMode > 19) && (ConfigMode < 30)) {
    if (22 == ConfigMode) ConfigMode = 128; else ConfigMode++;
  } else
    ConfigMode = 0;
}

void CheckCfgMdInc() {
  if ((ConfigMode > 9) && (ConfigMode < 20)) {
    if (11 == ConfigMode) ConfigMode = 20; else ConfigMode++;
  } else
    ConfigMode = 0;
}

void interrupt() {
  if (INTF_bit) { //rotary encoder push
    PushedNoRotate = 0b1;
    if (Power) RotHasPushed = 0b1; else RotateAct = ACT_NONE;
    NoActivity = NO_ACTIVITY_TIME;
    INTF_bit = 0b0;
  }
  if (GPIF_bit) { //rotary encoder rotate
    if ((LastClk != RotClk) || (LastDat != RotDat)) {
      Seq = (Seq << 1) | RotClk;
      Seq.B4 = RotDat;
      if (Seq == 0b00111001) {
        if (Power) {
          RotateAct = ACT_DOWN_DEC;
          if (RotPush) RotateAct += 2;
        } else if (!RotPush)
          RotateAct--;
        else
          CheckCfgMdDec();
      }
      if (Seq == 0b10010011) {
        if (Power) {
          RotateAct = ACT_DOWN_INC;
          if (RotPush) RotateAct += 2;
        } else if (!RotPush)
          RotateAct++;
        else
          CheckCfgMdInc();
      }
      LastClk = RotClk;
      LastDat = RotDat;
      PushedNoRotate = 0b0;
      NoActivity = NO_ACTIVITY_TIME;
    }   
    GPIF_bit = 0b0;
  }
  TMR0IE_bit = 0b1;
  if (TMR0IF_bit) {
    NoActivity--;
    if (!NoActivity) TMR0IE_bit = 0b0;
    TMR0IF_bit = 0b0;
  }
}

void main() {
  TRISIO = 0b111100;       // rotary encoder pins and reset button input, others output
  GPIO = 0b000011;         // high the TX pin and SIM800 power pin
  ANSEL = 0;               // all pins digital
  IOC = 0b110000;          // interrupt on rotate
  CMCON0 = 7;              // turn off comparator
  OPTION_REG = 0b10010111; // INT on falling edge, Timer0 internal clock source and 1:256 prescaler
  INTCON = 0b00011000;     // rotary encoder push and GPIO change interrupt
  
  Power = 0b0;
  RotateAct = ACT_NONE;
  NoActivity = 0;
  PushedNoRotate = 0b0;
  LastClk = RotClk;
  LastDat = RotDat;
  LastPush = 0b0;
  RotHasPushed = 0b0;
  Seq = 0;
  ConfigMode = 0;
  
  // if reset button is preesed activate reconfig mode
  if ((STATUS.B4) && (!STATUS.B3)) {
    EEPROM_Write(RECONFIG_ADDR, 255);
    Delay_ms(1000);
    BTMode = 0b1;
    SendAT(AT_AT, 1);
    SendAT(AT_AT, 1);
    SendAT(AT_AT, 1);
    TurnOff();
  }  
  EEADR = EEPROM_DUMMY_ADDR;
  
  GIE_bit = 0b1;
  while (1) {
    if (Power) {
      if ((RotHasPushed) && (!LastPush)) PushedNoRotate = 0b1;
      if ((!RotHasPushed) && (PushedNoRotate) && (LastPush)) {
        if (ConfigMode.B7) {
          if (CONFMOD_RINGTONE == ConfigMode) {
            SendAT(AT_Save, 0);
          }
          ConfigMode++;
          if (CONFMOD_LASTITEM + 1 == ConfigMode) {
            ConfigMode = 0;
            if (EEPROM_Read(BT_PAIRMODE_ADDR) != BTPairMode) {
              asm NOP;
              asm NOP;
              EEPROM_Write(RECONFIG_ADDR, 255);
              asm NOP;
              asm NOP;
              EEPROM_Write(BT_PAIRMODE_ADDR, BTPairMode);
            }
            EEPROM_Write(SPKVOL_ADDR, SpeakerVolume);
            asm NOP;
            asm NOP;
            EEPROM_Write(MICGAIN_ADDR, MicGain);
            asm NOP;
            asm NOP;
            EEPROM_Write(RINGTONE_ADDR, RingTone);
            asm NOP;
            asm NOP;
            EEPROM_Write(RINGERLEVEL_ADDR, RingerLevel);
            asm NOP;
            asm NOP;
            BTMode = 0b1;      
            EEADR = EEPROM_DUMMY_ADDR;
            TurnOff();
          } else
            PresentConfigVal();
        } else if (BTMode) {
          if (BTIsPlaying) SendAT(AT_BTPause, 0); else SendAT(AT_BTPlay, 0);
          BTIsPlaying = !BTIsPlaying;
        } else
          FMFreqChangeMode = !FMFreqChangeMode;
      }
      if ((!RotPush) && (PushedNoRotate) && (!NoActivity)) TurnOff();
      if (RotateAct < 10) {
        if (RotateAct > ACT_DOWN_DEC) {
          if (ConfigMode.B7) {
            if (CONFMOD_BTPAIRING == ConfigMode) {
              BTPairMode++;
              if (BTPairMode > 1) BTPairMode = 0;
            }  
            if (CONFMOD_SPKVOL == ConfigMode) {
              if (ACT_DEC == RotateAct) SpeakerVolume--; else SpeakerVolume++;
              if (255 == SpeakerVolume) SpeakerVolume = 0;
              if (10 == SpeakerVolume) SpeakerVolume = 9;
            }
            if (CONFMOD_MICGAIN == ConfigMode) {
              if (ACT_DEC == RotateAct) MicGain--; else MicGain++;
              if (255 == MicGain) MicGain = 0;
              if (10 == MicGain) MicGain = 9;
            }
            if (CONFMOD_RINGTONE == ConfigMode) {
              if (ACT_DEC == RotateAct) RingTone--; else RingTone++;
              if (0 == RingTone) RingTone = 19;
              if (20 == RingTone) RingTone = 1;
            }
            if (CONFMOD_RINGVOL == ConfigMode) {
              if (ACT_DEC == RotateAct) RingerLevel--; else RingerLevel++;
              if (255 == RingerLevel) RingerLevel = 0;
              if (10 == RingerLevel) RingerLevel = 9;
            }
            PresentConfigVal();
          } else if (BTMode)
             SendAT(AT_BTNext + RotateAct, 0);
          else {
            if (FMFreqChangeMode) {
              if (ACT_INC == RotateAct) FMFreq++;
              if (206 == FMFreq) FMFreq = 0;
              if (ACT_DEC == RotateAct) FMFreq--;
              if (255 == FMFreq) FMFreq = 205;
              SendAT(AT_FMFREQ, 0);
            } else {
              if (ACT_INC == RotateAct) FMVolume++;
              if (7 == FMVolume) FMVolume = 6;
              if (ACT_DEC == RotateAct) FMVolume--;
              if (255 == FMVolume) FMVolume = 0;
              SendAT(AT_FMVOLUME, 0);
            }
          }
          RotateAct = ACT_NONE;
        } else if ((!RotHasPushed) && (LastPush)) {
          if (BTMode) SendAT(AT_BTNext + RotateAct, 0);
          if (ACT_DOWN_DEC == RotateAct) {
#ifdef CanHaveSIMCard
            SendAT(AT_ATA, 0);
            Delay_ms(800);
#endif
            if (BTMode) SendAT(AT_BTATA, 0);
          }
          if (ACT_DOWN_INC == RotateAct) {
#ifdef CanHaveSIMCard
            SendAT(AT_ATH, 0);
            Delay_ms(800);
#endif
            if (BTMode) SendAT(AT_BTATH, 0);
          }
          RotateAct = ACT_NONE;
        }
      }
      if (RotHasPushed != LastPush) LastPush = RotHasPushed;
      if (RotPush) RotHasPushed = 0b0;
    } else if ((RotateAct < ACT_BTON) || (RotateAct > ACT_FMON) || (ConfigMode.B7))
      TurnOn();
    else if (!NoActivity) {
      asm SLEEP;
      asm NOP;
      ConfigMode = 0;
    }
  }
}