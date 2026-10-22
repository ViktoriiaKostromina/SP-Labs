#include "board.h"
#include "main.h"
#include <string.h>
static SPI_HandleTypeDef spi;
static ADC_HandleTypeDef adc;
UART_HandleTypeDef huart2;
static uint8_t rx_byte,rx_buffer[256];
static volatile uint16_t rx_read,rx_write;
static volatile uint32_t rx_overflow;
typedef struct { GPIO_TypeDef *port;uint16_t pin; } chip_select;
static chip_select rfid_cs={GPIOB,GPIO_PIN_0},sd_cs={GPIOA,GPIO_PIN_4};
static void fail(void){Error_Handler();}
static void clock_init(void)
{
    RCC_OscInitTypeDef o={0};RCC_ClkInitTypeDef c={0};
    o.OscillatorType=RCC_OSCILLATORTYPE_HSI;o.HSIState=RCC_HSI_ON;o.HSIDiv=RCC_HSI_DIV1;o.HSICalibrationValue=RCC_HSICALIBRATION_DEFAULT;
    if(HAL_RCC_OscConfig(&o)!=HAL_OK)fail();
    c.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
    c.SYSCLKSource=RCC_SYSCLKSOURCE_HSI;c.SYSCLKDivider=RCC_SYSCLK_DIV1;c.AHBCLKDivider=RCC_HCLK_DIV1;c.APB1CLKDivider=RCC_APB1_DIV1;
    if(HAL_RCC_ClockConfig(&c,FLASH_LATENCY_1)!=HAL_OK)fail();
}
static void gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0|GPIO_PIN_1,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8,GPIO_PIN_RESET);
    GPIO_InitTypeDef g={0};g.Pin=GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8;g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_HIGH;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_0|GPIO_PIN_1;HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_1|GPIO_PIN_6;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_0;g.Mode=GPIO_MODE_ANALOG;g.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_HIGH;g.Alternate=GPIO_AF0_SPI1;HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_4;g.Pull=GPIO_PULLDOWN;HAL_GPIO_Init(GPIOB,&g);
    /* Allow both SPI slaves to leave reset before the first transaction. */
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);HAL_Delay(2);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);HAL_Delay(50);
}
static void uart_init(void)
{
    huart2.Instance=USART2;huart2.Init.BaudRate=115200;huart2.Init.WordLength=UART_WORDLENGTH_8B;huart2.Init.StopBits=UART_STOPBITS_1;
    huart2.Init.Parity=UART_PARITY_NONE;huart2.Init.Mode=UART_MODE_TX_RX;huart2.Init.HwFlowCtl=UART_HWCONTROL_NONE;
    huart2.Init.OverSampling=UART_OVERSAMPLING_16;huart2.Init.OneBitSampling=UART_ONE_BIT_SAMPLE_DISABLE;huart2.Init.ClockPrescaler=UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit=UART_ADVFEATURE_NO_INIT;
    if(HAL_UART_Init(&huart2)!=HAL_OK)fail();
    HAL_NVIC_SetPriority(USART2_IRQn,1,0);HAL_NVIC_EnableIRQ(USART2_IRQn);
    if(HAL_UART_Receive_IT(&huart2,&rx_byte,1)!=HAL_OK)fail();
}
static void spi_init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();spi.Instance=SPI1;spi.Init.Mode=SPI_MODE_MASTER;spi.Init.Direction=SPI_DIRECTION_2LINES;
    spi.Init.DataSize=SPI_DATASIZE_8BIT;spi.Init.CLKPolarity=SPI_POLARITY_LOW;spi.Init.CLKPhase=SPI_PHASE_1EDGE;
    spi.Init.NSS=SPI_NSS_SOFT;spi.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_128;spi.Init.FirstBit=SPI_FIRSTBIT_MSB;
    spi.Init.TIMode=SPI_TIMODE_DISABLE;spi.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;spi.Init.CRCPolynomial=7;
    spi.Init.CRCLength=SPI_CRC_LENGTH_DATASIZE;spi.Init.NSSPMode=SPI_NSS_PULSE_DISABLE;
    if(HAL_SPI_Init(&spi)!=HAL_OK)fail();
}
static void adc_init(void)
{
    __HAL_RCC_ADC_CLK_ENABLE();adc.Instance=ADC1;adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution=ADC_RESOLUTION_12B;adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;adc.Init.ScanConvMode=ADC_SCAN_DISABLE;
    adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV;adc.Init.LowPowerAutoWait=DISABLE;adc.Init.LowPowerAutoPowerOff=DISABLE;
    adc.Init.ContinuousConvMode=DISABLE;adc.Init.NbrOfConversion=1;adc.Init.DiscontinuousConvMode=DISABLE;
    adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.DMAContinuousRequests=DISABLE;adc.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_79CYCLES_5;adc.Init.SamplingTimeCommon2=ADC_SAMPLETIME_79CYCLES_5;adc.Init.OversamplingMode=DISABLE;
    if(HAL_ADC_Init(&adc)!=HAL_OK)fail();
    ADC_ChannelConfTypeDef ch={0};ch.Channel=ADC_CHANNEL_0;ch.Rank=ADC_REGULAR_RANK_1;ch.SamplingTime=ADC_SAMPLINGTIME_COMMON_1;
    if(HAL_ADC_ConfigChannel(&adc,&ch)!=HAL_OK)fail();
    if(HAL_ADCEx_Calibration_Start(&adc)!=HAL_OK)fail();
}
static st_result exchange(void *ctx,const uint8_t *tx,uint8_t *rx,size_t n)
{
    (void)ctx;
    /* Keep 8-bit accesses explicit. The simulator does not reproduce the
       STM32 packed 16-bit RX FIFO read used by HAL for multi-byte buffers.
       CS remains asserted by the caller across the complete transaction. */
    for(size_t i=0;i<n;++i) {
        HAL_StatusTypeDef r=HAL_SPI_TransmitReceive(&spi,(uint8_t *)(tx+i),rx+i,1,10);
        if(r!=HAL_OK)return r==HAL_TIMEOUT?ST_TIMEOUT:ST_BUS;
    }
    return ST_OK;
}
static void select_chip(void *ctx,bool active)
{
    chip_select *cs=ctx;HAL_GPIO_WritePin(cs->port,cs->pin,active?GPIO_PIN_RESET:GPIO_PIN_SET);
}
static uint32_t time_for_bus(void *ctx){(void)ctx;return HAL_GetTick();}
static bool sample_adc(uint16_t *raw)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET);
    HAL_StatusTypeDef r=HAL_ADC_Start(&adc);
    if(r==HAL_OK)r=HAL_ADC_PollForConversion(&adc,2);
    if(r==HAL_OK)*raw=(uint16_t)HAL_ADC_GetValue(&adc);
    (void)HAL_ADC_Stop(&adc);HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_RESET);return r==HAL_OK;
}
static bool button(unsigned n){return HAL_GPIO_ReadPin(GPIOA,n?GPIO_PIN_6:GPIO_PIN_1)==GPIO_PIN_RESET;}
static bool receive(uint8_t *c)
{
    if(rx_read==rx_write)return false;
    *c=rx_buffer[rx_read];rx_read=(uint16_t)((rx_read+1u)&255u);return true;
}
static uint32_t lost(void){return rx_overflow;}
static void log_line(const char *text){(void)HAL_UART_Transmit(&huart2,(uint8_t *)text,(uint16_t)strlen(text),100);}
static void led(bool on){HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,on?GPIO_PIN_SET:GPIO_PIN_RESET);}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h)
{
    if(h!=&huart2)return;
    uint16_t next=(uint16_t)((rx_write+1u)&255u);
    if(next==rx_read)++rx_overflow;else {rx_buffer[rx_write]=rx_byte;rx_write=next;}
    (void)HAL_UART_Receive_IT(&huart2,&rx_byte,1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *h)
{
    if(h==&huart2){++rx_overflow;(void)HAL_UART_Receive_IT(&huart2,&rx_byte,1);}
}
void USART2_IRQHandler(void){HAL_UART_IRQHandler(&huart2);}
static void capture(bool on){HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,on?GPIO_PIN_SET:GPIO_PIN_RESET);}
void st_board_init(void){clock_init();gpio_init();uart_init();spi_init();adc_init();}
const st_platform *st_board_platform(void)
{
    static const st_platform p={
        .reader={&rfid_cs,exchange,select_chip,time_for_bus},.storage={&sd_cs,exchange,select_chip,time_for_bus},
        .now=HAL_GetTick,.adc=sample_adc,.button=button,.rx=receive,.rx_lost=lost,.log=log_line,.led=led,.capture=capture};
    return &p;
}
void Error_Handler(void){__disable_irq();for(;;){}}
