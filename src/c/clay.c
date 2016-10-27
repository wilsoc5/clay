#include "clay.h"
#include "hash.h"

#define SIMPLE_APP_MESSAGE_NAMESPACE ("CLAY")

static ClayCallbacks s_callbacks;

static uint32_t prv_hash_key(const char *key) {
  return hash((uint8_t *)key, strlen(key));
}

static bool prv_write(const char *key, const SimpleDictDataType type, const void *data, size_t size, void *context) {
  uint32_t persist_key = prv_hash_key(key);
  return persist_write_data(persist_key, data, size) == size;
}

static void prv_simple_app_message_received_callback(const SimpleDict *message, void *context) {
  if (!message) {
    return;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "CLAY: Received SimpleAppMessage");
  simple_dict_foreach(message, prv_write, NULL);
  s_callbacks.settings_updated(context);
}

static bool prv_read(const char *key, void *value_out, size_t size) {
  uint32_t persist_key = prv_hash_key(key);
  return persist_read_data(persist_key, value_out, size) == size;
}

// ----- PUBLIC API -----

void clay_init(uint32_t inbox_size, const ClayCallbacks *callbacks, void *context) {
  s_callbacks = *callbacks;

  const SimpleAppMessageCallbacks simple_app_message_callbacks = {
      .message_received = prv_simple_app_message_received_callback,
  };
  const bool register_success = simple_app_message_register_callbacks(
      SIMPLE_APP_MESSAGE_NAMESPACE, &simple_app_message_callbacks, context);

  if (!register_success) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to register callbacks for namespace %s", SIMPLE_APP_MESSAGE_NAMESPACE);
    return;
  }

  const bool request_inbox_size_success = simple_app_message_request_inbox_size(inbox_size);
  if (!request_inbox_size_success) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to request inbox size of %d", (int)inbox_size);
    return;
  }

  simple_app_message_open();
}

bool clay_get_int(const char *key, int *value_out) {
  return prv_read(key, value_out, sizeof(*value_out));
}

bool clay_get_bool(const char *key, bool *value_out) {
  return prv_read(key, value_out, sizeof(*value_out));
}

bool clay_get_string(const char *key, char *value_out, size_t size) {
  return prv_read(key, value_out, size);
}
