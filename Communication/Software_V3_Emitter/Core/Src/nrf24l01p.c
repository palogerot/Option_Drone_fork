#include "nrf24l01p.h"


static void cs_high()
{
    HAL_GPIO_WritePin(NRF24L01P_SPI_CS_PIN_PORT, NRF24L01P_SPI_CS_PIN_NUMBER, GPIO_PIN_SET);
}

static void cs_low()
{
    HAL_GPIO_WritePin(NRF24L01P_SPI_CS_PIN_PORT, NRF24L01P_SPI_CS_PIN_NUMBER, GPIO_PIN_RESET);
}

static void ce_high()
{
    HAL_GPIO_WritePin(NRF24L01P_CE_PIN_PORT, NRF24L01P_CE_PIN_NUMBER, GPIO_PIN_SET);
}

static void ce_low()
{
    HAL_GPIO_WritePin(NRF24L01P_CE_PIN_PORT, NRF24L01P_CE_PIN_NUMBER, GPIO_PIN_RESET);
}

static uint8_t read_register(uint8_t reg)
{
    uint8_t command = NRF24L01P_CMD_R_REGISTER | reg;
    uint8_t status;
    uint8_t read_val;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    HAL_SPI_Receive(NRF24L01P_SPI, &read_val, 1, 2000);
    cs_high();

    return read_val;
}

static uint8_t write_register(uint8_t reg, uint8_t value)
{
    uint8_t command = NRF24L01P_CMD_W_REGISTER | reg;
    uint8_t status;
    uint8_t write_val = value;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    HAL_SPI_Transmit(NRF24L01P_SPI, &write_val, 1, 2000);
    cs_high();

    return write_val;
}


/* nRF24L01+ Main Functions */
void nrf24l01p_rx_init(channel MHz, air_data_rate bps)
{
    nrf24l01p_reset();
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t cmd, st;

    cmd = NRF24L01P_CMD_W_REGISTER | NRF24L01P_REG_RX_ADDR_P1;
    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &cmd, &st, 1, 2000);
    HAL_SPI_Transmit(NRF24L01P_SPI, addr, 5, 2000);
    cs_high();
    nrf24l01p_prx_mode();
    nrf24l01p_power_up();

    HAL_Delay(2); // FIX: attendre 1.5ms minimum apres power_up (datasheet p.20)

    nrf24l01p_rx_set_payload_widths(NRF24L01P_PAYLOAD_LENGTH);

    nrf24l01p_set_rf_channel(MHz);
    nrf24l01p_set_rf_air_data_rate(bps);
    nrf24l01p_set_rf_tx_output_power(_0dBm);

    nrf24l01p_set_crc_length(1);
    nrf24l01p_set_address_widths(5);

    nrf24l01p_auto_retransmit_count(3);
    nrf24l01p_auto_retransmit_delay(500); // 500us pour 250kbps (datasheet recommande >500us)

    ce_high(); // Active le mode RX en continu
}

void nrf24l01p_tx_init(channel MHz, air_data_rate bps)
{
    nrf24l01p_reset();

    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t cmd, st;

    cmd = NRF24L01P_CMD_W_REGISTER | NRF24L01P_REG_TX_ADDR;
    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &cmd, &st, 1, 2000);
    HAL_SPI_Transmit(NRF24L01P_SPI, addr, 5, 2000);
    cs_high();

    cmd = NRF24L01P_CMD_W_REGISTER | NRF24L01P_REG_RX_ADDR_P0;
    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &cmd, &st, 1, 2000);
    HAL_SPI_Transmit(NRF24L01P_SPI, addr, 5, 2000);
    cs_high();
    nrf24l01p_ptx_mode();
    nrf24l01p_power_up();

    HAL_Delay(2); // FIX: attendre 1.5ms minimum apres power_up (datasheet p.20)

    nrf24l01p_set_rf_channel(MHz);
    nrf24l01p_set_rf_air_data_rate(bps);
    nrf24l01p_set_rf_tx_output_power(_0dBm);

    nrf24l01p_set_crc_length(1);
    nrf24l01p_set_address_widths(5);

    nrf24l01p_auto_retransmit_count(3);
    nrf24l01p_auto_retransmit_delay(500); // 500us pour 250kbps

    ce_low(); // FIX: rester en standby-I, CE pulse seulement lors de tx_transmit
}

void nrf24l01p_rx_receive(uint8_t* rx_payload)
{
    // FIX: supprime le toggle hardcode sur PC13, a gerer dans main.c
    nrf24l01p_read_rx_fifo(rx_payload);
    nrf24l01p_clear_rx_dr();
}

void nrf24l01p_tx_transmit(uint8_t* tx_payload)
{
    // FIX: pulse CE >= 10us pour declencher la transmission (datasheet p.34)
    ce_low();
    nrf24l01p_write_tx_fifo(tx_payload);
    ce_high();
    HAL_Delay(1); // > 10us
    ce_low();     // repasse en standby-I, attend IRQ (TX_DS ou MAX_RT)
}

void nrf24l01p_tx_irq()
{
    uint8_t status = nrf24l01p_get_status();

    if (status & 0x20)
    {
        // TX_DS : paquet envoye et acquitte
        nrf24l01p_clear_tx_ds();
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); // LED OK - a adapter selon votre board
    }
    else if (status & 0x10)
    {
        // MAX_RT : nombre max de retransmissions atteint, vider TX FIFO
        nrf24l01p_flush_tx_fifo();
        nrf24l01p_clear_max_rt();
        // FIX: la TX FIFO doit etre videe sinon le module reste bloque
    }
}

/* nRF24L01+ Sub Functions */
void nrf24l01p_reset()
{
    // Reset pins
    cs_high();
    ce_low();

    // Reset registers (valeurs datasheet page 54)
    write_register(NRF24L01P_REG_CONFIG,      0x08);
    write_register(NRF24L01P_REG_EN_AA,       0x3F);
    write_register(NRF24L01P_REG_EN_RXADDR,   0x03);
    write_register(NRF24L01P_REG_SETUP_AW,    0x03);
    write_register(NRF24L01P_REG_SETUP_RETR,  0x03);
    write_register(NRF24L01P_REG_RF_CH,       0x02);
    write_register(NRF24L01P_REG_RF_SETUP,    0x07);
    write_register(NRF24L01P_REG_STATUS,      0x7E);
    write_register(NRF24L01P_REG_RX_PW_P0,   0x00); // FIX: P0 une seule fois
    write_register(NRF24L01P_REG_RX_PW_P1,   0x00);
    write_register(NRF24L01P_REG_RX_PW_P2,   0x00);
    write_register(NRF24L01P_REG_RX_PW_P3,   0x00);
    write_register(NRF24L01P_REG_RX_PW_P4,   0x00);
    write_register(NRF24L01P_REG_RX_PW_P5,   0x00);
    write_register(NRF24L01P_REG_DYNPD,       0x00);
    write_register(NRF24L01P_REG_FEATURE,     0x00);

    // Reset FIFO
    nrf24l01p_flush_rx_fifo();
    nrf24l01p_flush_tx_fifo();
}

void nrf24l01p_prx_mode()
{
    uint8_t new_config = read_register(NRF24L01P_REG_CONFIG);
    new_config |= (1 << 0); // PRIM_RX = 1
    write_register(NRF24L01P_REG_CONFIG, new_config);
}

void nrf24l01p_ptx_mode()
{
    uint8_t new_config = read_register(NRF24L01P_REG_CONFIG);
    new_config &= 0xFE; // PRIM_RX = 0
    write_register(NRF24L01P_REG_CONFIG, new_config);
}

uint8_t nrf24l01p_read_rx_fifo(uint8_t* rx_payload)
{
    uint8_t command = NRF24L01P_CMD_R_RX_PAYLOAD;
    uint8_t status;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    HAL_SPI_Receive(NRF24L01P_SPI, rx_payload, NRF24L01P_PAYLOAD_LENGTH, 2000);
    cs_high();

    return status;
}

uint8_t nrf24l01p_write_tx_fifo(uint8_t* tx_payload)
{
    uint8_t command = NRF24L01P_CMD_W_TX_PAYLOAD;
    uint8_t status;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    HAL_SPI_Transmit(NRF24L01P_SPI, tx_payload, NRF24L01P_PAYLOAD_LENGTH, 2000);
    cs_high();

    return status;
}

void nrf24l01p_flush_rx_fifo()
{
    uint8_t command = NRF24L01P_CMD_FLUSH_RX;
    uint8_t status;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    cs_high();
}

void nrf24l01p_flush_tx_fifo()
{
    uint8_t command = NRF24L01P_CMD_FLUSH_TX;
    uint8_t status;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    cs_high();
}

uint8_t nrf24l01p_get_status()
{
    uint8_t command = NRF24L01P_CMD_NOP;
    uint8_t status;

    cs_low();
    HAL_SPI_TransmitReceive(NRF24L01P_SPI, &command, &status, 1, 2000);
    cs_high();

    return status;
}

uint8_t nrf24l01p_get_fifo_status()
{
    return read_register(NRF24L01P_REG_FIFO_STATUS);
}

void nrf24l01p_rx_set_payload_widths(widths bytes)
{
    write_register(NRF24L01P_REG_RX_PW_P1, bytes);
}

void nrf24l01p_clear_rx_dr()
{
    uint8_t new_status = nrf24l01p_get_status();
    new_status |= 0x40;
    write_register(NRF24L01P_REG_STATUS, new_status);
}

void nrf24l01p_clear_tx_ds()
{
    uint8_t new_status = nrf24l01p_get_status();
    new_status |= 0x20;
    write_register(NRF24L01P_REG_STATUS, new_status);
}

void nrf24l01p_clear_max_rt()
{
    uint8_t new_status = nrf24l01p_get_status();
    new_status |= 0x10;
    write_register(NRF24L01P_REG_STATUS, new_status);
}

void nrf24l01p_power_up()
{
    uint8_t new_config = read_register(NRF24L01P_REG_CONFIG);
    new_config |= (1 << 1); // PWR_UP = 1
    write_register(NRF24L01P_REG_CONFIG, new_config);
}

void nrf24l01p_power_down()
{
    uint8_t new_config = read_register(NRF24L01P_REG_CONFIG);
    new_config &= 0xFD; // PWR_UP = 0
    write_register(NRF24L01P_REG_CONFIG, new_config);
}

void nrf24l01p_set_crc_length(length bytes)
{
    uint8_t new_config = read_register(NRF24L01P_REG_CONFIG);

    switch (bytes)
    {
        case 1:
            new_config &= 0xFB; // CRCO = 0 => CRC 1 octet
            break;
        case 2:
            new_config |= (1 << 2); // CRCO = 1 => CRC 2 octets
            break;
    }

    write_register(NRF24L01P_REG_CONFIG, new_config);
}

void nrf24l01p_set_address_widths(widths bytes)
{
    write_register(NRF24L01P_REG_SETUP_AW, bytes - 2);
}

void nrf24l01p_auto_retransmit_count(count cnt)
{
    uint8_t new_setup_retr = read_register(NRF24L01P_REG_SETUP_RETR);
    new_setup_retr &= 0xF0;          // FIX: effacer ARC (bits 3:0) avant d'ecrire
    new_setup_retr |= (cnt & 0x0F);
    write_register(NRF24L01P_REG_SETUP_RETR, new_setup_retr);
}

void nrf24l01p_auto_retransmit_delay(delay us)
{
    uint8_t new_setup_retr = read_register(NRF24L01P_REG_SETUP_RETR);
    new_setup_retr &= 0x0F;                           // FIX: effacer ARD (bits 7:4) avant d'ecrire
    new_setup_retr |= (((us / 250) - 1) & 0x0F) << 4;
    write_register(NRF24L01P_REG_SETUP_RETR, new_setup_retr);
}

void nrf24l01p_set_rf_channel(channel MHz)
{
    // FIX: uint8_t au lieu de uint16_t, clamp a 125 (registre RF_CH = 7 bits)
    uint8_t new_rf_ch = (uint8_t)(MHz - 2400);
    if (new_rf_ch > 125) new_rf_ch = 125;
    write_register(NRF24L01P_REG_RF_CH, new_rf_ch);
}

void nrf24l01p_set_rf_tx_output_power(output_power dBm)
{
    uint8_t new_rf_setup = read_register(NRF24L01P_REG_RF_SETUP) & 0xF9;
    new_rf_setup |= (dBm << 1);
    write_register(NRF24L01P_REG_RF_SETUP, new_rf_setup);
}

void nrf24l01p_set_rf_air_data_rate(air_data_rate bps)
{
    uint8_t new_rf_setup = read_register(NRF24L01P_REG_RF_SETUP) & 0xD7;

    switch (bps)
    {
        case _1Mbps:
            break;
        case _2Mbps:
            new_rf_setup |= (1 << 3);
            break;
        case _250kbps:
            new_rf_setup |= (1 << 5);
            break;
    }

    write_register(NRF24L01P_REG_RF_SETUP, new_rf_setup);
}
