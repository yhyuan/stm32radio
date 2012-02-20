/*
 * File      : rtdef.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2011, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-16     aozima       the first version
 */

#include <stdint.h>
#include "spi_flash_sst25vfxx.h"

#define FLASH_DEBUG

#ifdef FLASH_DEBUG
#define FLASH_TRACE         rt_kprintf
#else
#define FLASH_TRACE(...)
#endif /**< #ifdef FLASH_DEBUG */

/* JEDEC Manufacturer��s ID */
#define MF_ID           (0xBF)
/* JEDEC Device ID : Memory Type */
#define MT_ID           (0x25)
/* JEDEC Device ID: Memory Capacity */
#define MC_ID_SST25VF016               (0x41)
#define MC_ID_SST25VF032               (0x4A)
#define MC_ID_SST25VF064               (0x4B)

/* command list */
#define CMD_RDSR                    (0x05)  /* ��״̬�Ĵ���     */
#define CMD_WRSR                    (0x01)  /* д״̬�Ĵ���     */
#define CMD_EWSR                    (0x50)  /* ʹ��д״̬�Ĵ��� */
#define CMD_WRDI                    (0x04)  /* �ر�дʹ��       */
#define CMD_WREN                    (0x06)  /* ��дʹ��       */
#define CMD_READ                    (0x03)  /* ������           */
#define CMD_FAST_READ               (0x0B)  /* ���ٶ�           */
#define CMD_BP                      (0x02)  /* �ֽڱ��         */
#define CMD_AAIP                    (0xAD)  /* �Զ���ַ������� */
#define CMD_ERASE_4K                (0x20)  /* ��������:4K      */
#define CMD_ERASE_32K               (0x52)  /* ��������:32K     */
#define CMD_ERASE_64K               (0xD8)  /* ��������:64K     */
#define CMD_ERASE_full              (0xC7)  /* ȫƬ����         */
#define CMD_JEDEC_ID                (0x9F)  /* �� JEDEC_ID      */
#define CMD_EBSY                    (0x70)  /* ��SOæ���ָʾ */
#define CMD_DBSY                    (0x80)  /* �ر�SOæ���ָʾ */

#define DUMMY                       (0xFF)

static struct spi_flash_device  spi_flash_device;

static uint8_t sst25vfxx_read_status(void)
{
    return rt_spi_sendrecv8(spi_flash_device.rt_spi_device, CMD_RDSR);
}

static void sst25vfxx_wait_busy(void)
{
    while( sst25vfxx_read_status() & (0x01));
}

/** \brief read [size] byte from [offset] to [buffer]
 *
 * \param offset uint32_t unit : byte
 * \param buffer uint8_t*
 * \param size uint32_t   unit : byte
 * \return uint32_t byte for read
 *
 */
static uint32_t sst25vfxx_read(uint32_t offset, uint8_t * buffer, uint32_t size)
{
    uint8_t send_buffer[4];

    send_buffer[0] = CMD_WRDI;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    send_buffer[0] = CMD_READ;
    send_buffer[1] = (uint8_t)(offset>>16);
    send_buffer[2] = (uint8_t)(offset>>8);
    send_buffer[3] = (uint8_t)(offset);
    rt_spi_send_then_recv(spi_flash_device.rt_spi_device, send_buffer, 4, buffer, size);

    return size;
}

/** \brief write N page on [page]
 *
 * \param page uint32_t unit : byte (4096 * N,1 page = 4096byte)
 * \param buffer const uint8_t*
 * \param size uint32_t unit : byte ( 4096*N )
 * \return uint32_t
 *
 */
uint32_t sst25vfxx_page_write(uint32_t page, const uint8_t * buffer, uint32_t size)
{
    uint32_t index;
    uint32_t need_wirte = size;
    uint8_t send_buffer[6];

    page &= ~0xFFF; // page size = 4096byte

    send_buffer[0] = CMD_WREN;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    send_buffer[0] = CMD_ERASE_4K;
    send_buffer[1] = (page >> 16);
    send_buffer[2] = (page >> 8);
    send_buffer[3] = (page);
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 4);

    sst25vfxx_wait_busy(); // wait erase done.

    send_buffer[0] = CMD_WREN;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    send_buffer[0] = CMD_AAIP;
    send_buffer[1] = (uint8_t)(page >> 16);
    send_buffer[2] = (uint8_t)(page >> 8);
    send_buffer[3] = (uint8_t)(page);
    send_buffer[4] = *buffer++;
    send_buffer[5] = *buffer++;
    need_wirte -= 2;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 6);

    sst25vfxx_wait_busy();

    for(index=0; index < need_wirte/2; index++)
    {
        send_buffer[0] = CMD_AAIP;
        send_buffer[1] = *buffer++;
        send_buffer[2] = *buffer++;
        rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 3);
        sst25vfxx_wait_busy();
    }

    send_buffer[0] = CMD_WRDI;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    return size;
}

/* RT-Thread device interface */
static rt_err_t sst25vfxx_flash_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t sst25vfxx_flash_open(rt_device_t dev, rt_uint16_t oflag)
{
    uint8_t send_buffer[2];

    send_buffer[0] = CMD_DBSY;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    send_buffer[0] = CMD_EWSR;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 1);

    send_buffer[0] = CMD_WRSR;
    send_buffer[1] = 0;
    rt_spi_send(spi_flash_device.rt_spi_device, send_buffer, 2);

    return RT_EOK;
}

static rt_err_t sst25vfxx_flash_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t sst25vfxx_flash_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) return -RT_ERROR;

        geometry->bytes_per_sector = spi_flash_device.geometry.bytes_per_sector;
        geometry->sector_count = spi_flash_device.geometry.sector_count;
        geometry->block_size = spi_flash_device.geometry.block_size;
    }

    return RT_EOK;
}

static rt_size_t sst25vfxx_flash_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    sst25vfxx_read(pos*spi_flash_device.geometry.bytes_per_sector, buffer, size*spi_flash_device.geometry.bytes_per_sector);
    return size;
}

static rt_size_t sst25vfxx_flash_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    sst25vfxx_page_write(pos*spi_flash_device.geometry.bytes_per_sector, buffer, size*spi_flash_device.geometry.bytes_per_sector);
    return size;
}

rt_err_t sst25vfxx_init(const char * flash_device_name, const char * spi_device_name)
{
    struct rt_spi_device * rt_spi_device;

    rt_spi_device = (struct rt_spi_device *)rt_device_find(spi_device_name);
    if(rt_spi_device == RT_NULL)
    {
        FLASH_TRACE("spi device %s not found!\r\n", spi_device_name);
        return -RT_ENOSYS;
    }
    spi_flash_device.rt_spi_device = rt_spi_device;

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible: Mode 0 and Mode 3 */
        cfg.max_hz = 50000000; /* 50M */
        rt_spi_configure(spi_flash_device.rt_spi_device, &cfg);
    }

    /* init flash */
    {
        rt_uint8_t cmd;
        rt_uint8_t id_recv[3];

        cmd = CMD_WRDI;
        rt_spi_send(spi_flash_device.rt_spi_device, &cmd, 1);

        /* read flash id */
        cmd = CMD_JEDEC_ID;
        rt_spi_send_then_recv(spi_flash_device.rt_spi_device, &cmd, 1, id_recv, 3);

        if(id_recv[0] != MF_ID || id_recv[1] != MT_ID)
        {
            FLASH_TRACE("Manufacturer��s ID or Memory Type error!\r\n");
            FLASH_TRACE("JEDEC Read-ID Data : %02X %02X %02X\r\n", id_recv[0], id_recv[1], id_recv[2]);
            return -RT_ENOSYS;
        }

        spi_flash_device.geometry.bytes_per_sector = 4096;
        spi_flash_device.geometry.block_size = 4096; /* block erase: 4k */

        if(id_recv[2] == MC_ID_SST25VF016)
        {
            FLASH_TRACE("SST25VF016 detection\r\n");
            spi_flash_device.geometry.sector_count = 512;
        }
        else if(id_recv[2] == MC_ID_SST25VF032)
        {
            FLASH_TRACE("SST25VF032 detection\r\n");
            spi_flash_device.geometry.sector_count = 1024;
        }
        else if(id_recv[2] == MC_ID_SST25VF064)
        {
            FLASH_TRACE("SST25VF064 detection\r\n");
            spi_flash_device.geometry.sector_count = 2048;
        }
        else
        {
            FLASH_TRACE("Memory Capacity error!\r\n");
            return -RT_ENOSYS;
        }
    }

    /* register device */
    spi_flash_device.flash_device.type    = RT_Device_Class_Block;
    spi_flash_device.flash_device.init    = sst25vfxx_flash_init;
    spi_flash_device.flash_device.open    = sst25vfxx_flash_open;
    spi_flash_device.flash_device.close   = sst25vfxx_flash_close;
    spi_flash_device.flash_device.read 	  = sst25vfxx_flash_read;
    spi_flash_device.flash_device.write   = sst25vfxx_flash_write;
    spi_flash_device.flash_device.control = sst25vfxx_flash_control;
    /* no private */
    spi_flash_device.flash_device.user_data = RT_NULL;

    rt_device_register(&spi_flash_device.flash_device, flash_device_name,
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);

    return RT_EOK;
}
