/**
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 #define NRFX_CHECK(NRFX_TWI0_ENABLED)
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdk_common.h"
#include "nrf.h"
#include "nrf_esb.h"
#include "nrf_error.h"
#include "nrf_esb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_delay.h"
#include "app_util.h"
#include "mpu6050moi.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
bool led=false;
bool ouvert;
static nrf_esb_payload_t        tx_payload = NRF_ESB_CREATE_PAYLOAD(0, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00);
static nrf_esb_payload_t        tx2_payload = NRF_ESB_CREATE_PAYLOAD(0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
static nrf_esb_payload_t        rx_payload;
  static int16_t AccValue[3], AccValueInit[3];
    static int16_t Differentiel[3];

void nrf_esb_event_handler_prx(nrf_esb_evt_t const * p_event)
{
      switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_DEBUG("TX SUCCESS EVENT");
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG("TX FAILED EVENT");
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            NRF_LOG_DEBUG("RX RECEIVED EVENT");
            if (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
            {
                // Set LEDs identical to the ones on the PTX.
                
               
                      nrf_gpio_pin_write(LED_1, !(rx_payload.data[1]%8>0 && rx_payload.data[1]%8<=4));
                       
                
                      nrf_gpio_pin_write(LED_2, !(rx_payload.data[1]%8>1 && rx_payload.data[1]%8<=5));
                    
                      
                      nrf_gpio_pin_write(LED_3, !(rx_payload.data[1]%8>2 && rx_payload.data[1]%8<=6));
                      
               nrf_gpio_pin_write(LED_4, !(rx_payload.data[1]%8>3));
                 
                NRF_LOG_DEBUG("Receiving packet: %02x", rx_payload.data[1]);
                led=true;
                //a=true;
            }
            break;
    }
}

void nrf_esb_event_handler_ptx(nrf_esb_evt_t const * p_event)
{
      switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_DEBUG("TX SUCCESS EVENT");
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG("TX FAILED EVENT");
            (void) nrf_esb_flush_tx();
            (void) nrf_esb_start_tx();
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            NRF_LOG_DEBUG("RX RECEIVED EVENT");
            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
            {
                if (rx_payload.length > 0)
                {
                    NRF_LOG_DEBUG("RX RECEIVED PAYLOAD");
                }
            }
            
            break;
    }
}

void clocks_start( void )
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;

    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}


void gpio_init( void )
{
    nrf_gpio_range_cfg_output(8, 15);
    bsp_board_init(BSP_INIT_LEDS);
}
uint32_t esb_init_prx( void )
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
   // nrf_esb_config.retransmit_delay         = 600;
    nrf_esb_config.payload_length           = 8;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler_prx;
    //nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PRX;
    nrf_esb_config.selective_auto_ack       = false;

    err_code = nrf_esb_init(&nrf_esb_config);

    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, NRF_ESB_PIPE_COUNT);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

uint32_t esb_init_ptx( void )
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.retransmit_delay         = 600;
   // nrf_esb_config.payload_length           = 8;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler_ptx;
    //nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PTX;
    nrf_esb_config.selective_auto_ack       = false;

    err_code = nrf_esb_init(&nrf_esb_config);

    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, NRF_ESB_PIPE_COUNT);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

typedef enum {
  I2c_connect,
  I2c_disconnect,
  Nrf_connection_ptx,
  Nrf_connection_prx,
  Nrf_disconnect,
  fin,

} property;
int a=0;
//0
uint16_t compteurBuzzer;
int main(void)
{
      property etat = Nrf_connection_prx;
    while(true)
      {
   
    switch (etat){
        case I2c_connect:
              bsp_board_init(BSP_INIT_LEDS);
            compteurBuzzer=0;
           ouvert = false;
          if(a==0){
           twi_master_init();
           }else{
            twi_master_enable();
           }
            while(mpu6050_init2() == false) // wait until MPU6050 sensor is successfully initialized
            {
              nrf_delay_ms(1000);
            }
            MPU6050_ReadAcc(&AccValueInit[0], &AccValueInit[1], &AccValueInit[2]);
           while(ouvert==false){
            MPU6050_ReadAcc(&AccValue[0], &AccValue[1], &AccValue[2]);
            Differentiel[0]=abs(AccValueInit[0]-AccValue[0]);
            Differentiel[1]=abs(AccValueInit[1]-AccValue[1]);
            Differentiel[2]=abs(AccValueInit[2]-AccValue[2]);
            
            if(Differentiel[0]>800 && Differentiel[1]>800 && Differentiel[2]>800){
              
                    etat = I2c_disconnect;
                  break;
                
            }else{

              bsp_board_led_invert(0);
               nrf_delay_us(500000);
                
            }
              if(compteurBuzzer>3){
                   ouvert=true;
                   etat=I2c_disconnect;
              }else{
                compteurBuzzer++;
              }
            }
            a++;
            break;
            
        case I2c_disconnect:
          twi_master_desable();
          etat = Nrf_connection_ptx ;
          break;
        case Nrf_connection_ptx:
             gpio_init();
              ret_code_t err_code;
              err_code = NRF_LOG_INIT(NULL);
              APP_ERROR_CHECK(err_code);

              NRF_LOG_DEFAULT_BACKENDS_INIT();

              clocks_start();

              err_code = esb_init_ptx();
              APP_ERROR_CHECK(err_code);
              NRF_LOG_DEBUG("Enhanced ShockBurst Transmitter Example started.");
              tx_payload.noack = false;
              while(true){
                 if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS)
          {
              // Toggle one of the LEDs.
              nrf_gpio_pin_write(LED_1, !(tx_payload.data[1]%8>0 && tx_payload.data[1]%8<=4));
              nrf_gpio_pin_write(LED_2, !(tx_payload.data[1]%8>1 && tx_payload.data[1]%8<=5));
              nrf_gpio_pin_write(LED_3, !(tx_payload.data[1]%8>2 && tx_payload.data[1]%8<=6));
              nrf_gpio_pin_write(LED_4, !(tx_payload.data[1]%8>3));
              tx_payload.data[1]++;
              etat = Nrf_disconnect;
              break;
          }
          else
          {
              NRF_LOG_WARNING("Sending packet failed");
             // printf("ici");
              etat = Nrf_disconnect;
              //break;
          }
             nrf_delay_us(500000);
            
          }
        case Nrf_connection_prx:
         

            gpio_init();
             
             NRF_LOG_INIT(NULL);
          

            NRF_LOG_DEFAULT_BACKENDS_INIT();

            clocks_start();

             esb_init_prx();
           
             nrf_esb_start_rx();
          
            bool a=true;
            while (a)
            {
                 bsp_board_led_invert(1);
                if (NRF_LOG_PROCESS() == false)
                {
                    __WFE();
                }
                if(led){
                        a=false;
                         etat=fin;
                         break;
                        
                         }
            }
            break;
        case Nrf_disconnect:
             nrf_esb_disable();
             etat = Nrf_connection_prx;
             break;
       case fin:
             nrf_esb_disable();
             etat =I2c_connect;
             break;
        }
          
          }

    
   
   
   
   /* while (true)
    {
        NRF_LOG_DEBUG("Transmitting packet %02x", tx_payload.data[1]);
       
        
       //  nrf_gpio_pin_write(LED_1, !(tx_payload.data[1]%8>0 && tx_payload.data[1]%8<=4));
         
               // printf("porte ouverte");
            //err_code = nrf_esb_start_tx();
         // APP_ERROR_CHECK(err_code);

           
          //nrf_esb_write_payload(&tx2_payload);
        //bsp_board_led_invert(1);
       /* if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS)
        {
            // Toggle one of the LEDs.
           
           nrf_gpio_pin_write(LED_2, !(tx_payload.data[1]%8>1 && tx_payload.data[1]%8<=5));
            nrf_gpio_pin_write(LED_3, !(tx_payload.data[1]%8>2 && tx_payload.data[1]%8<=6));
            nrf_gpio_pin_write(LED_4, !(tx_payload.data[1]%8>3));
            tx_payload.data[1]++;
        }
        else
        {
            NRF_LOG_WARNING("Sending packet failed");
        }
        
        
    
    
      
      nrf_delay_us(500000);
    }*/
    
    
    
}
