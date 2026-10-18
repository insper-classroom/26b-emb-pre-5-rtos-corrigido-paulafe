#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueBtn;   // ISR -> btn_task (qual botão)
QueueHandle_t xQueueLedR;  // btn_task -> led_r_task (1 = pisca, 0 = para)
QueueHandle_t xQueueLedY;  // btn_task -> led_y_task

void btn_callback(uint gpio, uint32_t events) {
    if (events == GPIO_IRQ_EDGE_FALL) {
        int id = gpio;
        xQueueSendFromISR(xQueueBtn, &id, 0);
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int id;
    int estado_r = 0;  // variáveis LOCAIS (não globais): permitido
    int estado_y = 0;

    while (true) {
        if (xQueueReceive(xQueueBtn, &id, portMAX_DELAY)) {
            if (id == BTN_PIN_R) {
                estado_r = !estado_r;
                xQueueSend(xQueueLedR, &estado_r, 0);
            } else if (id == BTN_PIN_Y) {
                estado_y = !estado_y;
                xQueueSend(xQueueLedY, &estado_y, 0);
            }
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int piscando = 0;
    while (true) {
        if (xQueueReceive(xQueueLedR, &piscando, 0)) {
            if (!piscando) gpio_put(LED_PIN_R, 0);  // garante apagado
        }

        if (piscando) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));  // dorme e não gasta CPU
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int piscando = 0;
    while (true) {
        if (xQueueReceive(xQueueLedY, &piscando, 0)) {
            if (!piscando) gpio_put(LED_PIN_Y, 0);
        }

        if (piscando) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xQueueLedR = xQueueCreate(32, sizeof(int));
    xQueueLedY = xQueueCreate(32, sizeof(int));

    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_R_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }

    return 0;
}