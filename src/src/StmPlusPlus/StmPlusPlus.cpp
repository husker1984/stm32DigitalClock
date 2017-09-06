/*******************************************************************************
 * StmPlusPlus: object-oriented library implementing device drivers for 
 * STM32F3 and STM32F4 MCU
 * *****************************************************************************
 * Copyright (C) 2016-2017 Mikhail Kulesh
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <cstring>
#include <cstdlib>

#include "StmPlusPlus.h"

using namespace StmPlusPlus;

#define USART_DEBUG_MODULE "COMM: "

/************************************************************************
 * Class System
 ************************************************************************/

uint32_t System::externalOscillatorFreq = 16000000;
uint32_t System::mcuFreq = 16000000;

void System::setClock (uint32_t pllDiv, uint32_t pllMUL, uint32_t FLatency, RtcType rtcType, int32_t msAdjustment)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    #ifdef STM32F3
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = pllDiv;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = pllMUL;
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    #endif

    #ifdef STM32F4
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = pllDiv;
    RCC_OscInitStruct.PLL.PLLN = pllMUL;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    #ifdef STM32F410
    RCC_OscInitStruct.PLL.PLLR = 2;
    #endif
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    #endif

    RCC_PeriphCLKInitTypeDef PeriphClkInit;
    switch (rtcType)
    {
    case RtcType::RTC_INT:
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_LSI;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
        break;
    case RtcType::RTC_EXT:
        RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStruct.LSEState = RCC_LSE_ON;
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        break;
    case RtcType::RTC_NONE:
        // nothing to do
        break;
    }

    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLatency);

    /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
    if (HAL_GetREVID() == 0x1001)
    {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    }

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    externalOscillatorFreq = HSE_VALUE;
    mcuFreq = HAL_RCC_GetHCLKFreq();

    HAL_SYSTICK_Config(mcuFreq/1000 + msAdjustment);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/************************************************************************
 * Class IOPort
 ************************************************************************/

IOPort::IOPort (
        PortName name,
        uint32_t mode,
        uint32_t pull/* = GPIO_NOPULL*/,
        uint32_t speed/* = GPIO_SPEED_HIGH*/,
        uint32_t pin/* = GPIO_PIN_All*/,
        bool callInit/* = true*/)
{
    gpioParameters.Pin   = pin;
    gpioParameters.Mode  = mode;
    gpioParameters.Pull  = pull;
    gpioParameters.Speed = speed;

    switch (name)
    {
        case A:
            __GPIOA_CLK_ENABLE();
            port = GPIOA;
            break;
        case B:
            __GPIOB_CLK_ENABLE();
            port = GPIOB;
            break;
        case C:
            __GPIOC_CLK_ENABLE();
            port = GPIOC;
            break;
        case D:
            #ifdef GPIOD
            __GPIOD_CLK_ENABLE();
            port = GPIOD;
            #endif
            break;
        case F:
            #ifdef GPIOF
            __GPIOF_CLK_ENABLE();
            port = GPIOF;
            #endif
            break;
    }
    HAL_GPIO_DeInit(port, pin);
    if (callInit)
    {
        HAL_GPIO_Init(port, &gpioParameters);
    }
}

void IOPin::activateClockOutput(uint32_t source, uint32_t div)
{
  gpioParameters.Mode = GPIO_MODE_AF_PP;
  gpioParameters.Pull = GPIO_NOPULL;
  gpioParameters.Speed = GPIO_SPEED_FREQ_LOW;
  gpioParameters.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &gpioParameters);
  HAL_RCC_MCOConfig(RCC_MCO1, source, div);
}

/************************************************************************
 * Class Usart
 ************************************************************************/

Usart::Usart (DeviceName device, PortName name, uint32_t txPin, uint32_t rxPin):
    IOPort(name, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, txPin | rxPin, false)
{
    switch (device)
    {
    case USART_1:
        setAlternate(GPIO_AF7_USART1);
        enableClock = []() {
            __HAL_RCC_USART1_CLK_ENABLE();
            };
        disableClock = []() {
            __HAL_RCC_USART1_CLK_DISABLE();
            };
        usartParameters.Instance = USART1;
        break;

    case USART_2:
        setAlternate(GPIO_AF7_USART2);
        enableClock = []() {
            __HAL_RCC_USART2_CLK_ENABLE();
            };
        disableClock = []() {
            __HAL_RCC_USART2_CLK_DISABLE();
            };
        usartParameters.Instance = USART2;
        break;
    }

    usartParameters.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    usartParameters.Init.OverSampling = UART_OVERSAMPLING_16;
    #ifdef STM32F3
    usartParameters.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    usartParameters.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    #endif
}


HAL_StatusTypeDef Usart::start (uint32_t mode,
                   uint32_t baudRate,
                   uint32_t wordLength/* = UART_WORDLENGTH_8B*/,
                   uint32_t stopBits/* = UART_STOPBITS_1*/,
                   uint32_t parity/* = UART_PARITY_NONE*/)
{
    enableClock();
    usartParameters.Init.Mode = mode;
    usartParameters.Init.BaudRate = baudRate;
    usartParameters.Init.WordLength = wordLength;
    usartParameters.Init.StopBits = stopBits;
    usartParameters.Init.Parity = parity;
    return HAL_UART_Init(&usartParameters);
}


HAL_StatusTypeDef Usart::stop ()
{
    HAL_StatusTypeDef retValue = HAL_UART_DeInit(&usartParameters);
    disableClock();
    return retValue;
}


HAL_StatusTypeDef Usart::transmit (const char * buffer)
{
    return HAL_UART_Transmit(&usartParameters, (unsigned char *)buffer, ::strlen(buffer), 0xFFFF);
}


/************************************************************************
 * Class UsartLogger
 ************************************************************************/

UsartLogger * UsartLogger::instance = NULL;


UsartLogger::UsartLogger (DeviceName device, PortName name, uint32_t txPin, uint32_t rxPin, uint32_t _baudRate):
    Usart(device, name, txPin, rxPin),
    baudRate(_baudRate)
{
    // empty
}

UsartLogger & UsartLogger::operator << (const char * buffer)
{
    transmit(buffer);
    return *this;
}

UsartLogger & UsartLogger::operator << (int n)
{
    char buffer[1024];
    ::__itoa(n, buffer, 10);
    transmit(buffer);
    return *this;
}

UsartLogger & UsartLogger::operator << (Manupulator /*m*/)
{
    transmit("\n\r");
    return *this;
}


/************************************************************************
 * Class Timer
 ************************************************************************/
#define INITIALIZE_STMPLUSPLUS_TIMER(name, enableName) { \
        enableName(); \
        timerParameters.Instance = name; \
        }


Timer::Timer (TimerName timerName, IRQn_Type _irqName):
    handler(NULL),
    irqName(_irqName)
{
    switch (timerName)
    {
    case TIM_1:
        #ifdef TIM1
        INITIALIZE_STMPLUSPLUS_TIMER(TIM1, __TIM1_CLK_ENABLE);
        #endif
        break;
    case TIM_2:
        #ifdef TIM2
        INITIALIZE_STMPLUSPLUS_TIMER(TIM2, __TIM2_CLK_ENABLE);
        #endif
        break;
    case TIM_3:
        #ifdef TIM3
        INITIALIZE_STMPLUSPLUS_TIMER(TIM3, __TIM3_CLK_ENABLE);
        #endif
        break;
    case TIM_4:
        #ifdef TIM4
        INITIALIZE_STMPLUSPLUS_TIMER(TIM4, __TIM4_CLK_ENABLE);
        #endif
        break;
    case TIM_5:
        #ifdef TIM5
        INITIALIZE_STMPLUSPLUS_TIMER(TIM5, __TIM5_CLK_ENABLE);
        #endif
        break;
    case TIM_6:
        #ifdef TIM6
        INITIALIZE_STMPLUSPLUS_TIMER(TIM6, __TIM6_CLK_ENABLE);
        #endif
        break;
    case TIM_7:
        #ifdef TIM7
        INITIALIZE_STMPLUSPLUS_TIMER(TIM7, __TIM7_CLK_ENABLE);
        #endif
        break;
    case TIM_8:
        #ifdef TIM8
        INITIALIZE_STMPLUSPLUS_TIMER(TIM8, __TIM8_CLK_ENABLE);
        #endif
        break;
    case TIM_9:
        #ifdef TIM9
        INITIALIZE_STMPLUSPLUS_TIMER(TIM9, __TIM9_CLK_ENABLE);
        #endif
        break;
    case TIM_10:
        #ifdef TIM10
        INITIALIZE_STMPLUSPLUS_TIMER(TIM10, __TIM10_CLK_ENABLE);
        #endif
        break;
    case TIM_11:
        #ifdef TIM11
        INITIALIZE_STMPLUSPLUS_TIMER(TIM11, __TIM11_CLK_ENABLE);
        #endif
        break;
    case TIM_12:
        #ifdef TIM12
        INITIALIZE_STMPLUSPLUS_TIMER(TIM12, __TIM12_CLK_ENABLE);
        #endif
        break;
    case TIM_13:
        #ifdef TIM13
        INITIALIZE_STMPLUSPLUS_TIMER(TIM13, __TIM13_CLK_ENABLE);
        #endif
        break;
    case TIM_14:
        #ifdef TIM10
        INITIALIZE_STMPLUSPLUS_TIMER(TIM14, __TIM14_CLK_ENABLE);
        #endif
        break;
    case TIM_15:
        #ifdef TIM15
        INITIALIZE_STMPLUSPLUS_TIMER(TIM15, __TIM15_CLK_ENABLE);
        #endif
        break;
    case TIM_16:
        #ifdef TIM16
        INITIALIZE_STMPLUSPLUS_TIMER(TIM16, __TIM16_CLK_ENABLE);
        #endif
        break;
    case TIM_17:
        #ifdef TIM17
        INITIALIZE_STMPLUSPLUS_TIMER(TIM17, __TIM17_CLK_ENABLE);
        #endif
        break;
    }
}


void Timer::setPrescaler (uint32_t prescaler)
{
    timerParameters.Init.Prescaler = prescaler;
    __HAL_TIM_SET_PRESCALER(&timerParameters, prescaler);
}


HAL_StatusTypeDef Timer::start (uint32_t counterMode,
                         uint32_t prescaler,
                         uint32_t period,
                         uint32_t clockDivision/* = TIM_CLOCKDIVISION_DIV1*/,
                         uint32_t repetitionCounter/* = 1*/)
{
    timerParameters.Init.CounterMode = counterMode;
    timerParameters.Init.Prescaler = prescaler;
    timerParameters.Init.Period = period;
    timerParameters.Init.ClockDivision = clockDivision;
    timerParameters.Init.RepetitionCounter = repetitionCounter;
    __HAL_TIM_SET_COUNTER(&timerParameters, 0);

    HAL_TIM_Base_Init(&timerParameters);
    return HAL_TIM_Base_Start_IT(&timerParameters);
}


void Timer::startInterrupt (const InterruptPriority & prio, Timer::EventHandler * _handler /*= NULL*/)
{
    handler = _handler;
    HAL_NVIC_SetPriority(irqName, prio.first, prio.second);
    HAL_NVIC_EnableIRQ(irqName);
}


void Timer::processInterrupt () const
{
    if (__HAL_TIM_GET_FLAG(&timerParameters, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&timerParameters, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_IT(&timerParameters, TIM_IT_UPDATE);
            if (handler != NULL)
            {
                handler->onTimerUpdate(this);
            }
        }
    }
}


HAL_StatusTypeDef Timer::stop ()
{
    HAL_NVIC_DisableIRQ(irqName);
    HAL_TIM_Base_Stop_IT(&timerParameters);
    handler = NULL;
    return HAL_TIM_Base_DeInit(&timerParameters);
}


#undef INITIALIZE_STMPLUSPLUS_TIMER
#undef INITIALIZE_STMPLUSPLUS_THANDLER



/************************************************************************
 * Class Rtc
 ************************************************************************/

RealTimeClock::RealTimeClock ():
    handler(NULL),
    syncMs1(0),
    syncMs2(0),
    errorMs(0),
    timeMillisec(0),
    timeSec(0)
{
    rtcParameters.Instance = RTC;
    rtcParameters.Init.HourFormat = RTC_HOURFORMAT_24;
    rtcParameters.Init.AsynchPrediv = 127;
    rtcParameters.Init.SynchPrediv = 255;
    rtcParameters.Init.OutPut = RTC_OUTPUT_DISABLE;
    rtcParameters.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtcParameters.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    timeParameters.Hours = 0x0;
    timeParameters.Minutes = 0x0;
    timeParameters.Seconds = 0x0;
    timeParameters.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    timeParameters.StoreOperation = RTC_STOREOPERATION_RESET;

    dateParameters.WeekDay = RTC_WEEKDAY_MONDAY;
    dateParameters.Month = RTC_MONTH_JANUARY;
    dateParameters.Date = 0x1;
    dateParameters.Year = 0x0;
    
    HAL_RTCEx_DeactivateWakeUpTimer(&rtcParameters);
    HAL_RTC_DeInit(&rtcParameters);
    __HAL_RCC_RTC_DISABLE();
}


HAL_StatusTypeDef RealTimeClock::start (uint32_t counter, uint32_t prescaler, const InterruptPriority & prio, RealTimeClock::EventHandler * _handler /*= NULL*/)
{
    __HAL_RCC_RTC_ENABLE();

    HAL_StatusTypeDef status = HAL_RTC_Init(&rtcParameters);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize RTC: " << status);
        return status;
    }

    HAL_RTC_SetTime(&rtcParameters, &timeParameters, RTC_FORMAT_BCD);
    HAL_RTC_SetDate(&rtcParameters, &dateParameters, RTC_FORMAT_BCD);

    status = HAL_RTCEx_SetWakeUpTimer_IT(&rtcParameters, counter, prescaler);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize RTC WakeUpTimer: " << status);
        return status;
    }

    handler = _handler;
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, prio.first, prio.second);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    USART_DEBUG("Started RTC"
             << ": Counter = " << counter
             << ", Prescaler = " << prescaler
             << ", timeSec = " << timeSec
              << ", irqPrio = " << prio.first << "," << prio.second
             << ", Status = " << status);

    return status;
}


void RealTimeClock::onMilliSecondInterrupt ()
{
    HAL_IncTick();
    if (syncMs1 < 999)
    {
        ++timeMillisec;
        ++syncMs1;
    }
    ++syncMs2;
}


void RealTimeClock::onSecondInterrupt ()
{
    /* Get the pending status of the WAKEUPTIMER Interrupt */
    if(__HAL_RTC_WAKEUPTIMER_GET_FLAG(&rtcParameters, RTC_FLAG_WUTF) != RESET)
    {
        ++syncMs2;
        errorMs = syncMs2 - 1000;
        ++timeSec;
        timeMillisec = (time_ms)timeSec * 1000L;
        syncMs1 = syncMs2 = 0;

        if (handler != NULL)
        {
            handler->onRtcWakeUp();
        }
        /* Clear the WAKEUPTIMER interrupt pending bit */
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&rtcParameters, RTC_FLAG_WUTF);
    }

    /* Clear the EXTI's line Flag for RTC WakeUpTimer */
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
}


void RealTimeClock::stop ()
{
    USART_DEBUG("Stopping RTC");

    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
    HAL_RTCEx_DeactivateWakeUpTimer(&rtcParameters);
    HAL_RTC_DeInit(&rtcParameters);
    __HAL_RCC_RTC_DISABLE();
    handler = NULL;
}


/************************************************************************
 * Class Spi
 ************************************************************************/

Spi::Spi (DeviceName _device,
          IOPort::PortName sckPort,  uint32_t sckPin,
          IOPort::PortName misoPort, uint32_t misoPin,
          IOPort::PortName mosiPort, uint32_t mosiPin,
          uint32_t pull):
    device(_device),
    sck(sckPort,   sckPin,  GPIO_MODE_AF_PP, pull, GPIO_SPEED_HIGH, false),
    miso(misoPort, misoPin, GPIO_MODE_AF_PP, pull, GPIO_SPEED_HIGH, false),
    mosi(mosiPort, mosiPin, GPIO_MODE_AF_PP, pull, GPIO_SPEED_HIGH, false),
    hspi(NULL)
{
    switch (device)
    {
    case SPI_1:
        #ifdef SPI1
        sck.setAlternate(GPIO_AF5_SPI1);
        miso.setAlternate(GPIO_AF5_SPI1);
        mosi.setAlternate(GPIO_AF5_SPI1);
        spiParams.Instance = SPI1;
        enableClock = []() {
            __HAL_RCC_SPI1_CLK_ENABLE();
            };
        disableClock = []() {
            __HAL_RCC_SPI1_CLK_DISABLE();
            };
        #endif
        break;
    case SPI_2:
        #ifdef SPI2
        sck.setAlternate(GPIO_AF5_SPI2);
        miso.setAlternate(GPIO_AF5_SPI2);
        mosi.setAlternate(GPIO_AF5_SPI2);
        spiParams.Instance = SPI2;
        enableClock = []() {
            __HAL_RCC_SPI2_CLK_ENABLE();
            };
        disableClock = []() {
            __HAL_RCC_SPI2_CLK_DISABLE();
            };
        #endif
        break;
    case SPI_3:
        #ifdef SPI3
        sck.setAlternate(GPIO_AF6_SPI3);
        miso.setAlternate(GPIO_AF6_SPI3);
        mosi.setAlternate(GPIO_AF6_SPI3);
        spiParams.Instance = SPI3;
        enableClock = []() {
            __HAL_RCC_SPI3_CLK_ENABLE();
            };
        disableClock = []() {
            __HAL_RCC_SPI3_CLK_DISABLE();
            };
        #endif
        break;
    }

    spiParams.Init.Mode = SPI_MODE_MASTER;
    spiParams.Init.DataSize = SPI_DATASIZE_8BIT;
    spiParams.Init.CLKPolarity = SPI_POLARITY_HIGH;
    spiParams.Init.CLKPhase = SPI_PHASE_1EDGE;
    spiParams.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spiParams.Init.TIMode = SPI_TIMODE_DISABLE;
    spiParams.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spiParams.Init.CRCPolynomial = 7;
    spiParams.Init.NSS = SPI_NSS_SOFT;
    #ifdef STM32F3
    spiParams.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    spiParams.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    #endif
}


HAL_StatusTypeDef Spi::start (uint32_t direction, uint32_t prescaler, uint32_t dataSize, uint32_t CLKPhase)
{
    hspi = &spiParams;
    enableClock();

    spiParams.Init.Direction = direction;
    spiParams.Init.BaudRatePrescaler = prescaler;
    spiParams.Init.DataSize = dataSize;
    spiParams.Init.CLKPhase = CLKPhase;
    HAL_StatusTypeDef status = HAL_SPI_Init(hspi);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize SPI " << (size_t)device << ": " << status);
        return status;
    }

    /* Configure communication direction : 1Line */
    if (spiParams.Init.Direction == SPI_DIRECTION_1LINE)
    {
        SPI_1LINE_TX(hspi);
    }

    /* Check if the SPI is already enabled */
    if ((spiParams.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
    {
        /* Enable SPI peripheral */
        __HAL_SPI_ENABLE(hspi);
    }

    USART_DEBUG("Started SPI " << (size_t)device
             << ": BaudRatePrescaler = " << spiParams.Init.BaudRatePrescaler
             << ", DataSize = " << spiParams.Init.DataSize
             << ", CLKPhase = " << spiParams.Init.CLKPhase
             << ", Status = " << status);

    return status;
}


HAL_StatusTypeDef Spi::stop ()
{
    USART_DEBUG("Stopping SPI " << (size_t)device);
    HAL_StatusTypeDef retValue = HAL_SPI_DeInit(&spiParams);
    disableClock();
    hspi = NULL;
    return retValue;
}


/************************************************************************
 * Class PeriodicalEvent
 ************************************************************************/
PeriodicalEvent::PeriodicalEvent (const RealTimeClock & _rtc, time_ms _delay, long _maxOccurrence /* = -1*/):
    rtc(_rtc),
    lastEventTime(0),
    delay(_delay),
    maxOccurrence(_maxOccurrence),
    occurred(0)
{
    // empty
}


void PeriodicalEvent::resetTime ()
{
    lastEventTime = rtc.getTimeMillisec();
    occurred = 0;
}


bool PeriodicalEvent::isOccured ()
{
    if (maxOccurrence > 0 && occurred >= maxOccurrence)
    {
        return false;
    }
    if (rtc.getTimeMillisec() >= lastEventTime + delay)
    {
        lastEventTime = rtc.getTimeMillisec();
        if (maxOccurrence > 0)
        {
            ++occurred;
        }
        return true;
    }
    return false;
}


/************************************************************************
 * Class AnalogToDigitConverter
 ************************************************************************/

AnalogToDigitConverter::AnalogToDigitConverter (PortName name, uint32_t pin, DeviceName _device, uint32_t channel, float _vRef):
    IOPin(name, pin, GPIO_MODE_ANALOG, GPIO_NOPULL),
    device(_device),
    hadc(NULL),
    vRef(_vRef)
{
    switch (device)
    {
    case ADC_1:
        #ifdef ADC1
        adcParams.Instance = ADC1;
        enableClock = []() {
            __ADC1_CLK_ENABLE();
            };
        disableClock = []() {
            __ADC1_CLK_DISABLE();
            };
        #endif
        break;
    case ADC_2:
        #ifdef ADC2
        adcParams.Instance = ADC2;
        enableClock = []() {
            __ADC2_CLK_ENABLE();
            };
        disableClock = []() {
            __ADC2_CLK_DISABLE();
            };
        #endif
        break;
    case ADC_3:
        #ifdef ADC3
        adcParams.Instance = ADC3;
        enableClock = []() {
            __ADC3_CLK_ENABLE();
            };
        disableClock = []() {
            __ADC3_CLK_DISABLE();
            };
        #endif
        break;
    }

    adcParams.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    adcParams.Init.Resolution = ADC_RESOLUTION_12B;
    adcParams.Init.ScanConvMode = DISABLE;
    adcParams.Init.ContinuousConvMode = DISABLE;
    adcParams.Init.DiscontinuousConvMode = DISABLE; // enable if multi-channel conversion needed
    adcParams.Init.NbrOfDiscConversion = 0;
    adcParams.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adcParams.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    adcParams.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adcParams.Init.NbrOfConversion = 1;
    adcParams.Init.DMAContinuousRequests = DISABLE;
    adcParams.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    adcChannel.Channel = channel; // each channel is connected to the specific pin, see pin descriptions
    adcChannel.Rank = 1;
    adcChannel.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    adcChannel.Offset = 0;

    HAL_ADC_DeInit(&adcParams);
    disableClock();

    // for Multichannel ADC reading, see
    // https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FMultichannel%20ADC%20reading&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=105
}


HAL_StatusTypeDef AnalogToDigitConverter::start ()
{
    hadc = &adcParams;
    enableClock();

    HAL_StatusTypeDef status = HAL_ADC_Init(hadc);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not initialize ACD " << (size_t)device << ": " << status);
        return status;
    }

    status = HAL_ADC_ConfigChannel(hadc, &adcChannel);
    if (status != HAL_OK)
    {
        USART_DEBUG("Can not configure ACD channel " << adcChannel.Channel << ": " << status);
        return status;
    }

    USART_DEBUG("Started ACD " << (size_t)device
             << ": channel = " << adcChannel.Channel
             << ", Status = " << status);

    return status;
}


HAL_StatusTypeDef AnalogToDigitConverter::stop ()
{
    USART_DEBUG("Stopping ADC " << (size_t)device);
    HAL_StatusTypeDef retValue = HAL_ADC_DeInit(&adcParams);
    disableClock();
    hadc = NULL;
    return retValue;
}


uint32_t AnalogToDigitConverter::getValue ()
{
    uint32_t value = INVALID_VALUE;
    if (hadc == NULL)
    {
        return value;
    }
    if (HAL_ADC_Start(hadc) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(hadc, 10000) == HAL_OK)
        {
            value = HAL_ADC_GetValue(hadc);
        }
    }
    HAL_ADC_Stop(hadc);
    return value;
}


float AnalogToDigitConverter::getVoltage ()
{
    return (vRef * (float)getValue())/4095.0;
}
