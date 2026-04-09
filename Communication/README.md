# STM32 nRF24L01+ – Transmitter / Receiver (Unified)

* TURGUT Ibrahim
* PREVOST Alexian
* Mbassi Ewolo Loic Aron

Ce projet implémente une communication sans fil entre deux cartes STM32 via un module nRF24L01+.

Le code permet de fonctionner soit en **émetteur (TRANSMITTER)** soit en **récepteur (RECEIVER)** avec un seul fichier `main.c`.

---

## Fonctionnalités

* Mode **TX / RX configurable à la compilation**
* Gestion par **interruptions (IRQ)**
* Interface UART pour communication avec PC (Python, terminal…)
* Retour d’état transmission :

  * `TX_OK`
  * `TX_ECHEC`
  * `TIMEOUT`
* Indication visuelle via LED (LD2)


## Principe de fonctionnement

### Mode TRANSMITTER

1. Réception de données via UART (interruption)
2. Envoi via nRF24L01+
3. Attente du résultat via IRQ
4. Retour du statut via UART


### Mode RECEIVER

1. Attente d’un paquet radio (IRQ)
2. Lecture du payload
3. Affichage via UART
4. Toggle LED


## Configuration du mode

Dans `main.c` :

```c
#define TRANSMITTER
// #define RECEIVER
```

## 🔌 Connexions principales

| nRF24L01+ | STM32     |
| --------- | --------- |
| VCC       | 3.3V    |
| GND       | GND       |
| CE        | GPIO      |
| CSN       | GPIO      |
| SCK       | SPI       |
| MOSI      | SPI       |
| MISO      | SPI       |
| IRQ       | GPIO EXTI |



## Gestion des interruptions

### UART

```c
HAL_UART_RxCpltCallback()
```

→ Déclenche l’envoi radio côté TX

---

### IRQ nRF24L01+

```c
HAL_GPIO_EXTI_Callback()
```

* **TX :**

  * `TX_DS` → succès
  * `MAX_RT` → échec

* **RX :**

  * lecture des données reçues

---

## Paramètres radio

```c
nrf24l01p_tx_init(2500, _250kbps);
nrf24l01p_rx_init(2500, _250kbps);
```

##  Communication UART

* Baudrate : **115200**
* Format : 8N1

### TX → PC

* `TX_OK`
* `TX_ECHEC`
* `TIMEOUT`

### RX → PC

```
Recu: <message>
```

---

## Gestion des erreurs

En cas de timeout :

* lecture du registre STATUS
* nettoyage FIFO TX
* reset des flags

```c
if (s & 0x10) { nrf24l01p_flush_tx_fifo(); nrf24l01p_clear_max_rt(); }
if (s & 0x20) { nrf24l01p_clear_tx_ds(); }
```

  
