/*
 * Copyright (c) 2017-2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <fsl_clock.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <fsl_flexspi_nor_boot.h>
#if CONFIG_USB_DC_NXP_EHCI
#include "usb_phy.h"
#include "usb_dc_mcux.h"
#endif

#ifdef CONFIG_INIT_ARM_PLL
/* ARM PLL configuration for RUN mode */
const clock_arm_pll_config_t armPllConfig = {
	.loopDivider = 100U
};
#endif

#if CONFIG_USB_DC_NXP_EHCI
/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif

#ifdef CONFIG_INIT_ENET_PLL
/* ENET PLL configuration for RUN mode */
const clock_enet_pll_config_t ethPllConfig = {
#if defined(CONFIG_SOC_MIMXRT1011) || \
	defined(CONFIG_SOC_MIMXRT1015) || \
	defined(CONFIG_SOC_MIMXRT1021) || \
	defined(CONFIG_SOC_MIMXRT1024)
	.enableClkOutput500M = true,
#endif
#ifdef CONFIG_ETH_MCUX
	.enableClkOutput = true,
#endif
#if defined(CONFIG_PTP_CLOCK_MCUX)
	.enableClkOutput25M = true,
#else
	.enableClkOutput25M = false,
#endif
	.loopDivider = 1,
};
#endif

#if CONFIG_USB_DC_NXP_EHCI
	usb_phy_config_struct_t usbPhyConfig = {
		BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
	};
#endif

#ifdef CONFIG_INIT_VIDEO_PLL
const clock_video_pll_config_t videoPllConfig = {
	.loopDivider = 31,
	.postDivider = 8,
	.numerator = 0,
	.denominator = 0,
};
#endif

#ifdef CONFIG_NXP_IMX_RT_BOOT_HEADER
const __imx_boot_data_section BOOT_DATA_T boot_data = {
	.start = CONFIG_FLASH_BASE_ADDRESS,
	.size = KB(CONFIG_FLASH_SIZE),
	.plugin = PLUGIN_FLAG,
	.placeholder = 0xFFFFFFFF,
};

const __imx_boot_ivt_section ivt image_vector_table = {
	.hdr = IVT_HEADER,
	.entry = (uint32_t) _vector_start,
	.reserved1 = IVT_RSVD,
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	.dcd = (uint32_t) dcd_data,
#else
	.dcd = (uint32_t) NULL,
#endif
	.boot_data = (uint32_t) &boot_data,
	.self = (uint32_t) &image_vector_table,
	.csf = (uint32_t)CSF_ADDRESS,
	.reserved2 = IVT_RSVD,
};
#endif

/**
 * @brief Initialize the system clock
 */
static ALWAYS_INLINE void clock_init(void)
{
	/* Boot ROM did initialize the XTAL, here we only sets external XTAL
	 * OSC freq
	 */
	CLOCK_SetXtalFreq(24000000U);
	CLOCK_SetRtcXtalFreq(32768U);

	/* Set PERIPH_CLK2 MUX to OSC */
	CLOCK_SetMux(kCLOCK_PeriphClk2Mux, 0x1);

	/* Set PERIPH_CLK MUX to PERIPH_CLK2 */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0x1);

	/* Setting the VDD_SOC value.
	 */
	DCDC->REG3 = (DCDC->REG3 & (~DCDC_REG3_TRG_MASK)) | DCDC_REG3_TRG(CONFIG_DCDC_VALUE);
	/* Waiting for DCDC_STS_DC_OK bit is asserted */
	while (DCDC_REG0_STS_DC_OK_MASK !=
			(DCDC_REG0_STS_DC_OK_MASK & DCDC->REG0)) {
		;
	}

#ifdef CONFIG_INIT_ARM_PLL
	CLOCK_InitArmPll(&armPllConfig); /* Configure ARM PLL to 1200M */
#endif
#ifdef CONFIG_INIT_ENET_PLL
	CLOCK_InitEnetPll(&ethPllConfig);
#endif
#ifdef CONFIG_INIT_VIDEO_PLL
	CLOCK_InitVideoPll(&videoPllConfig);
#endif

#ifdef CONFIG_HAS_ARM_DIV
	CLOCK_SetDiv(kCLOCK_ArmDiv, CONFIG_ARM_DIV); /* Set ARM PODF */
#endif
	CLOCK_SetDiv(kCLOCK_AhbDiv, CONFIG_AHB_DIV); /* Set AHB PODF */
	CLOCK_SetDiv(kCLOCK_IpgDiv, CONFIG_IPG_DIV); /* Set IPG PODF */

	/* Set PRE_PERIPH_CLK to PLL1, 1200M */
	CLOCK_SetMux(kCLOCK_PrePeriphMux, 0x3);

	/* Set PERIPH_CLK MUX to PRE_PERIPH_CLK */
	CLOCK_SetMux(kCLOCK_PeriphMux, 0x0);

#ifdef CONFIG_UART_MCUX_LPUART
	/* Configure UART divider to default */
	CLOCK_SetMux(kCLOCK_UartMux, 0); /* Set UART source to PLL3 80M */
	CLOCK_SetDiv(kCLOCK_UartDiv, 0); /* Set UART divider to 1 */
#endif

#ifdef CONFIG_I2C_MCUX_LPI2C
	CLOCK_SetMux(kCLOCK_Lpi2cMux, 0); /* Set I2C source as USB1 PLL 480M */
	CLOCK_SetDiv(kCLOCK_Lpi2cDiv, 5); /* Set I2C divider to 6 */
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
	CLOCK_SetMux(kCLOCK_LpspiMux, 1); /* Set SPI source to USB1 PFD0 720M */
	CLOCK_SetDiv(kCLOCK_LpspiDiv, 7); /* Set SPI divider to 8 */
#endif

#ifdef CONFIG_DISPLAY_MCUX_ELCDIF
	CLOCK_SetMux(kCLOCK_LcdifPreMux, 2);
	CLOCK_SetDiv(kCLOCK_LcdifPreDiv, 4);
	CLOCK_SetDiv(kCLOCK_LcdifDiv, 1);
#endif

#if CONFIG_USB_DC_NXP_EHCI
	CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usb480M,
		DT_PROP_BY_PHANDLE(DT_INST(0, nxp_mcux_usbd), clocks, clock_frequency));
	CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M,
		DT_PROP_BY_PHANDLE(DT_INST(0, nxp_mcux_usbd), clocks, clock_frequency));
	USB_EhciPhyInit(kUSB_ControllerEhci0, CPU_XTAL_CLK_HZ, &usbPhyConfig);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC
	/* Configure USDHC clock source and divider */
	CLOCK_SetDiv(kCLOCK_Usdhc1Div, 1U);
	CLOCK_SetMux(kCLOCK_Usdhc1Mux, 1U);
	CLOCK_EnableClock(kCLOCK_Usdhc1);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc2), okay) && CONFIG_DISK_DRIVER_SDMMC
	/* Configure USDHC clock source and divider */
	CLOCK_SetDiv(kCLOCK_Usdhc2Div, 1U);
	CLOCK_SetMux(kCLOCK_Usdhc2Mux, 1U);
	CLOCK_EnableClock(kCLOCK_Usdhc2);
#endif

#ifdef CONFIG_VIDEO_MCUX_CSI
	CLOCK_EnableClock(kCLOCK_Csi); /* Disable CSI clock gate */
	CLOCK_SetDiv(kCLOCK_CsiDiv, 0); /* Set CSI divider to 1 */
	CLOCK_SetMux(kCLOCK_CsiMux, 0); /* Set CSI source to OSC 24M */
#endif
#ifdef CONFIG_CAN_MCUX_FLEXCAN
	CLOCK_SetDiv(kCLOCK_CanDiv, 1); /* Set CAN_CLK_PODF. */
	CLOCK_SetMux(kCLOCK_CanMux, 2); /* Set Can clock source. */
#endif

	/* Keep the system clock running so SYSTICK can wake up the system from
	 * wfi.
	 */
	CLOCK_SetMode(kCLOCK_ModeRun);

}

#if (DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC)

/* Usdhc driver needs to re-configure pinmux
 * Pinmux depends on board design.
 * From the perspective of Usdhc driver,
 * it can't access board specific function.
 * So SoC provides this for board to register
 * its usdhc pinmux and for usdhc to access
 * pinmux.
 */

static usdhc_pin_cfg_cb g_usdhc_pin_cfg_cb;

void imxrt_usdhc_pinmux_cb_register(usdhc_pin_cfg_cb cb)
{
	g_usdhc_pin_cfg_cb = cb;
}

void imxrt_usdhc_pinmux(uint16_t nusdhc, bool init,
	uint32_t speed, uint32_t strength)
{
	if (g_usdhc_pin_cfg_cb)
		g_usdhc_pin_cfg_cb(nusdhc, init,
			speed, strength);
}

/* Usdhc driver needs to reconfigure the dat3 line to a pullup in order to
 * detect an SD card on the bus. Expose a callback to do that here. The board
 * must register this callback in its init function.
 */
static usdhc_dat3_cfg_cb g_usdhc_dat3_cfg_cb;

void imxrt_usdhc_dat3_cb_register(usdhc_dat3_cfg_cb cb)
{
	g_usdhc_dat3_cfg_cb = cb;
}

void imxrt_usdhc_dat3_pull(bool pullup)
{
	if (g_usdhc_dat3_cfg_cb) {
		g_usdhc_dat3_cfg_cb(pullup);
	}
}

#endif

#if CONFIG_I2S_MCUX_SAI
void imxrt_audio_codec_pll_init(uint32_t clock_name, uint32_t clk_src,
					uint32_t clk_pre_div, uint32_t clk_src_div)
{
	switch (clock_name) {
	case IMX_CCM_SAI1_CLK:
		CLOCK_SetMux(kCLOCK_Sai1Mux, clk_src);
		CLOCK_SetDiv(kCLOCK_Sai1PreDiv, clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai1Div, clk_src_div);
		break;
	case IMX_CCM_SAI2_CLK:
		CLOCK_SetMux(kCLOCK_Sai2Mux, clk_src);
		CLOCK_SetDiv(kCLOCK_Sai2PreDiv, clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai2Div, clk_src_div);
		break;
	case IMX_CCM_SAI3_CLK:
		CLOCK_SetMux(kCLOCK_Sai2Mux, clk_src);
		CLOCK_SetDiv(kCLOCK_Sai2PreDiv, clk_pre_div);
		CLOCK_SetDiv(kCLOCK_Sai2Div, clk_src_div);
		break;
	default:
		LOG_ERR("wrong clock system configured");
		return;
	}
}
#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int imxrt_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Watchdog disable */
	if ((WDOG1->WCR & WDOG_WCR_WDE_MASK) != 0) {
		WDOG1->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	if ((WDOG2->WCR & WDOG_WCR_WDE_MASK) != 0) {
		WDOG2->WCR &= ~WDOG_WCR_WDE_MASK;
	}

	RTWDOG->CNT = 0xD928C520U; /* 0xD928C520U is the update key */
	RTWDOG->TOVAL = 0xFFFF;
	RTWDOG->CS = (uint32_t) ((RTWDOG->CS) & ~RTWDOG_CS_EN_MASK)
		| RTWDOG_CS_UPDATE_MASK;

	/* Disable Systick which might be enabled by bootrom */
	if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}

	SCB_EnableICache();
	if ((SCB->CCR & SCB_CCR_DC_Msk) == 0) {
		SCB_EnableDCache();
	}

	/* Initialize system clock */
	clock_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(imxrt_init, PRE_KERNEL_1, 0);
