/*
 * My_ethrnet.c
 *
 *  Created on: Aug 30, 2026
 *      Author: rishi
 */

#include "fsl_enet.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_phylan8741.h"
#include "My_ethrnet.h"
/*${header:end}*/

/*${variable:start}*/
phy_lan8741_resource_t g_phy_resource;

static void MDIO_Init(void)
{
    (void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(EXAMPLE_ENET_BASE)]);
    ENET_SetSMI(EXAMPLE_ENET_BASE, CLOCK_GetCoreSysClkFreq());
}

static status_t MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
    return ENET_MDIOWrite(EXAMPLE_ENET_BASE, phyAddr, regAddr, data);
}

static status_t MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData)
{
    return ENET_MDIORead(EXAMPLE_ENET_BASE, phyAddr, regAddr, pData);
}


void Ethrnet_MDIO_Init()
{
    MDIO_Init();

    g_phy_resource.read  = MDIO_Read;
    g_phy_resource.write = MDIO_Write;
}
