#ifndef __VL53L0X_H__
#define __VL53L0X_H__

/**
 *  SENSOR VL53L0X - Sensor de distância por tempo de voo
 * 
*/

#include <stdint.h>
#include <stdio.h>

// Endereço padrão do VL53L0X
#define VL53L0X_ADDR  0x29UL

// Registradores importantes
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xC0
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GP1     0x0A
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR          0x0B
#define VL53L0X_REG_SYSTEM_MODE_GP1                 0x11
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14
#define VL53L0X_REG_SYSRANGE_START                  0x00
#define VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT        0x01

// Status codes
#define VL53L0X_STATUS_OK              0x00
#define VL53L0X_STATUS_TIMEOUT         0x01
#define VL53L0X_STATUS_ERROR           0x02

// Lê byte do VL53L0X
uint8_t vl53l0x_read(uint8_t reg) {
    uint8_t value;
    i2c_read(VL53L0X_ADDR, reg, &value, 1, true, 1);
    return value;
}

// Lê múltiplos bytes
void vl53l0x_read_buf(uint8_t reg, uint8_t *data, uint8_t len) {
    i2c_read(VL53L0X_ADDR, reg, data, len, true, 1);
}

// Escreve byte no VL53L0X
void vl53l0x_write(uint8_t reg, uint8_t value) {
    i2c_write(VL53L0X_ADDR, reg, &value, 1, 1);
}

// Inicializa VL53L0X
uint8_t vl53l0x_init() {
    // Verifica se o sensor está presente
    uint8_t model_id = vl53l0x_read(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
    
    if (model_id != 0xEE) {
        return VL53L0X_STATUS_ERROR;
    }
    
    // Configuração básica
    vl53l0x_write(0x88, 0x00);
    vl53l0x_write(0x80, 0x01);
    vl53l0x_write(0xFF, 0x01);
    vl53l0x_write(0x00, 0x00);
    
    // Para o sensor
    vl53l0x_write(VL53L0X_REG_SYSRANGE_START, 0x01);
    
    // Limpa interrupção
    vl53l0x_write(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    // Configura modo single shot
    vl53l0x_write(VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT, 0x01);
    
    // Configura timing budget (20ms)
    vl53l0x_write(0x0D, 0x14);
    
    return VL53L0X_STATUS_OK;
}

// Inicia medição
void vl53l0x_start_measurement() {
    vl53l0x_write(VL53L0X_REG_SYSRANGE_START, 0x01);
}

// Verifica se medição está pronta
uint8_t vl53l0x_is_ready() {
    uint8_t status = vl53l0x_read(VL53L0X_REG_RESULT_INTERRUPT_STATUS);
    return (status & 0x07) != 0;
}

// Lê distância em mm (com timeout)
uint16_t vl53l0x_read_distance(uint8_t *status) {
    uint8_t data[2];
    uint32_t timeout = 1000; // timeout em ms
    
    // Aguarda medição estar pronta com timeout
    while (!vl53l0x_is_ready()) {
        timeout--;
        if (timeout == 0) {
            *status = VL53L0X_STATUS_TIMEOUT;
            return 0xFFFF;
        }
        delay_ms(1);
    }
    
    // Lê os bytes de distância
    vl53l0x_read_buf(0x14, data, 2);
    
    // Limpa interrupção
    vl53l0x_write(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    // Converte para valor de 16 bits
    uint16_t distance = (data[0] << 8) | data[1];
    
    *status = VL53L0X_STATUS_OK;
    return distance;
}

// Formata distância para exibição (sem ponto flutuante)
void format_distance(uint16_t distance_mm, char *buffer) {
    if (distance_mm > 2000) {
        // Distância muito longa ou erro
        sprintf(buffer, ">2m");
    }
    else if (distance_mm > 1000) {
        // Exibe em metros (ex: 1234 mm -> 1.23 m)
        uint16_t meters = distance_mm / 1000;
        uint16_t centimeters = (distance_mm % 1000) / 10;
        sprintf(buffer, "%d.%02d m", meters, centimeters);
    }
    else if (distance_mm > 100) {
        // Exibe em centímetros (ex: 234 mm -> 23 cm)
        uint16_t cm = distance_mm / 10;
        sprintf(buffer, "%d cm", cm);
    }
    else {
        // Exibe em milímetros
        sprintf(buffer, "%d mm", distance_mm);
    }
}

// Função simplificada para leitura única
uint16_t vl53l0x_read_distance_simple() {
    uint8_t status;
    vl53l0x_start_measurement();
    return vl53l0x_read_distance(&status);
}

// Verifica se a distância é válida
uint8_t vl53l0x_is_valid_distance(uint16_t distance) {
    return (distance < 2000); // Distâncias acima de 2m podem ser inválidas
}

#endif