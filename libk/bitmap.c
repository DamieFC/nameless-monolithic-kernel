#include <libk/bitmap.h>
#include <libk/logging.h>
#include <memory/pmm.h>
#include <system/panic.h>

static void set(size_t index, bool value, Bitmap *self)
{
    if (index > self->size)
    {
        log(ERROR, "Trying to set bit outside of the bitmap! %x > %x", index, self->size);

        panic("Trying to set bit outside of the bitmap!");

        return;
    }

    size_t bit = index % 8; /* 8 since the bitmap is uint8_t */
    size_t byte = index / 8;

    if (value)
    {
        self->buffer[byte] |= (1 << (bit));
    }
    else
    {
        self->buffer[byte] &= ~(1 << (bit));
    }
}

static bool get(size_t index, Bitmap *self)
{
    if (index > self->size)
    {
        log(ERROR, "Trying to get bit outside of the bitmap!");

        panic("Trying to get bit outside of the bitmap!");

        return false;
    }

    size_t bit = index % 8; /* 8 since the bitmap is uint8_t */
    size_t byte = index / 8;

    return (self->buffer[byte] & (1 << (bit)));
}

static size_t find_free(size_t length, Bitmap *self)
{
    size_t current_found_length = 0;
    size_t current_found_index = 0;

    size_t i;

    for (i = self->last_free; i < self->size; i++)
    {
        if (i == 0)
        {
            continue;
        }

        if (!self->get(i, self))
        {
            if (current_found_length == 0)
            {
                current_found_index = i;
            }
            current_found_length++;
        }

        else
        {
            current_found_length = 0;
            current_found_index = 0;
        }

        if (current_found_length == length)
        {
            self->last_free = current_found_index + current_found_length;
            return current_found_index;
        }
    }

    if (self->last_free == 0)
    {
        panic("No free entry found for memory allocation");
        return 0;
    }
    else
    {
        self->last_free = 0;
        return find_free(length, self);
    }
}

static size_t allocate(size_t length, Bitmap *self)
{
    size_t block = self->find_free(length, self);

    if (block == 0)
    {
        log(WARNING, "Can't allocate blocks");

        return 0;
    }

    if (self->set_used(block, length, self) == 0)
    {
        log(WARNING, "Can't set blocks as used");
        return 0;
    }

    return block;
}

static size_t set_free(size_t index, size_t length, Bitmap *self)
{
    size_t i;
    for (i = 0; i < length; i++)
    {
        log(INFO, "%d", index + i);

        self->set(index + i, false, self);
    }

    self->last_free = index;
    return 1;
}

static size_t set_used(size_t index, size_t length, Bitmap *self)
{
    size_t i;
    for (i = 0; i < length; i++)
    {
        self->set(index + i, true, self);
    }

    return 1;
}

Bitmap _Bitmap(uint8_t *data, size_t size)
{
    module("Bitmap");

    log(INFO, "Creating bitmap at address %x which is %d bytes big", data, size / 8);

    memset(data, 0xff, size);

    Bitmap new_bitmap;

    new_bitmap.size = size;
    new_bitmap.buffer = data;
    new_bitmap.last_free = 0;
    new_bitmap.set = set;
    new_bitmap.get = get;
    new_bitmap.find_free = find_free;
    new_bitmap.allocate = allocate;
    new_bitmap.set_free = set_free;
    new_bitmap.set_used = set_used;

    log(INFO, "Bitmap created!");

    return new_bitmap;
}
