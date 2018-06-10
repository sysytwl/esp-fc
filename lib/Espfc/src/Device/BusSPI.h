#ifndef _ESPFC_DEVICE_BUSSPI_H_
#define _ESPFC_DEVICE_BUSSPI_H_

#include "BusDevice.h"
#include "Debug.h"
#include <SPI.h>

namespace Espfc {

namespace Device {

class BusSPI: public BusDevice
{
  public:
    static const uint8_t SPI_READ = 0x80;

    int begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1)
    {
      if(sck == -1 || miso == -1 || mosi == -1) return 0;
      
#if defined(ESP32)
      SPI.begin(sck, miso, mosi, ss);
#elif defined(ESP8266)
      SPI.pins(sck, miso, mosi, ss);
      SPI.begin();
#endif

      return 1;
    }

    int8_t read(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data, uint16_t timeout = ESPFC_BUS_TIMEOUT) override
    {
      transfer(devAddr, regAddr | SPI_READ, length, NULL, data);
      //D("spi:r", regAddr, length, *data);
      return length;
    }

    bool write(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data) override
    {
      //D("spi:w", regAddr, length, *data);
      transfer(devAddr, regAddr, length, data, NULL);
      return true;
    }

  private:
    void transfer(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *in, uint8_t *out)
    {
      //SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
      //SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
      SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE3));
      digitalWrite(devAddr, LOW);
      SPI.transfer(regAddr); // specify the starting register address
      //SPI.transferBytes(in, out, length);
      for(uint8_t i = 0; i < length; i++)
      {
        uint8_t v = SPI.transfer(in ? in[i] : 0);
        if(out) out[i] = v; // write the data
      }
      digitalWrite(devAddr, HIGH);
      SPI.endTransaction();
    }
};

}

}

#endif