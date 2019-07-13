#include "hawktracer/listeners/file_dump_listener.h"
#include "hawktracer/alloc.h"
#include "hawktracer/timeline_listener.h"

#include "internal/listener_buffer.h"
#include "internal/mutex.h"

struct _HT_FileDumpListener
{
    HT_ListenerBuffer buffer;
    FILE* p_file;
    HT_Mutex* mtx;
};

inline static void
_ht_file_dump_listener_flush(void* listener, HT_Byte* data, size_t size)
{
    HT_FileDumpListener* fd_listener = (HT_FileDumpListener*) listener;

    fwrite(data, sizeof(HT_Byte), size, fd_listener->p_file);
}

HT_FileDumpListener*
ht_file_dump_listener_create(const char* filename, size_t buffer_size, HT_ErrorCode* out_err)
{
    HT_ErrorCode error_code = HT_ERR_OK;
    HT_FileDumpListener* listener = HT_CREATE_TYPE(HT_FileDumpListener);

    if (listener == NULL)
    {
        error_code = HT_ERR_OUT_OF_MEMORY;
        goto done;
    }

    listener->p_file = fopen(filename, "wb");
    if (listener->p_file == NULL)
    {
        error_code = HT_ERR_CANT_OPEN_FILE;
        goto error_open_file;
    }

    listener->mtx = ht_mutex_create();
    if (listener->mtx == NULL)
    {
        error_code = HT_ERR_OUT_OF_MEMORY;
        goto error_create_mutex;
    }

    error_code = ht_listener_buffer_init(&listener->buffer, buffer_size);
    if (error_code != HT_ERR_OK)
    {
        ht_mutex_destroy(listener->mtx);
    }
    else
    {
        ht_timeline_listener_push_metadata(ht_file_dump_listener_callback, listener, HT_TRUE);
        goto done;
    }

error_create_mutex:
    fclose(listener->p_file);

error_open_file:
    ht_free(listener);
    listener = NULL;

done:
    if (out_err != NULL)
    {
        *out_err = error_code;
    }

    return listener;
}

void
ht_file_dump_listener_destroy(HT_FileDumpListener* listener)
{
    if (listener != NULL)
    {
        ht_mutex_lock(listener->mtx);
        ht_listener_buffer_flush(&listener->buffer, _ht_file_dump_listener_flush, listener);
        ht_listener_buffer_deinit(&listener->buffer);
        fclose(listener->p_file);
        listener->p_file = NULL;
        ht_mutex_unlock(listener->mtx);
        ht_mutex_destroy(listener->mtx);
        ht_free(listener);
    }
}

HT_ErrorCode
ht_file_dump_listener_flush(HT_FileDumpListener* listener, HT_Boolean flush_stream)
{
    HT_ErrorCode ret = HT_ERR_OK;

    ht_mutex_lock(listener->mtx);

    ht_listener_buffer_flush(&listener->buffer, _ht_file_dump_listener_flush, listener);

    if (flush_stream == HT_TRUE)
    {
        if (fflush(listener->p_file) != 0)
        {
            ret = HT_ERR_UNKNOWN;
        }
    }

    ht_mutex_unlock(listener->mtx);

    return ret;
}

void
ht_file_dump_listener_callback(TEventPtr events, size_t size, HT_Boolean serialized, void* user_data)
{
    HT_FileDumpListener* listener = (HT_FileDumpListener*)user_data;

    ht_mutex_lock(listener->mtx);

    if (serialized)
    {
        ht_listener_buffer_process_serialized_events(&listener->buffer, events, size, _ht_file_dump_listener_flush, listener);
    }
    else
    {
        ht_listener_buffer_process_unserialized_events(&listener->buffer, events, size, _ht_file_dump_listener_flush, listener);
    }

    ht_mutex_unlock(listener->mtx);
}
