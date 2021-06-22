// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "share_plus_tizen_plugin.h"

#include <app_control.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

#include "log.h"

#define RET_IF_ERROR(ret)                                       \
  if (ret != APP_CONTROL_ERROR_NONE) {                          \
    result->Error(std::to_string(ret), get_error_message(ret)); \
    if (handle) {                                               \
      app_control_destroy(handle);                              \
    }                                                           \
    return;                                                     \
  }

class SharePlusTizenPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar) {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "dev.fluttercommunity.plus/share",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<SharePlusTizenPlugin>();

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  SharePlusTizenPlugin() {}

  virtual ~SharePlusTizenPlugin() {}

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    LOG_INFO("method : %s", method_call.method_name().data());

    if (method_call.method_name().compare("share") != 0 &&
        method_call.method_name().compare("shareFiles") != 0) {
      result->Error("-1", "Not supported method");
      return;
    }

    std::string text, subject, mime_type;
    std::vector<const char *> path_array;
    flutter::EncodableMap arguments;
    try {
      if (method_call.arguments()) {
        arguments = std::get<flutter::EncodableMap>(*method_call.arguments());
        auto subject_value = arguments[flutter::EncodableValue("subject")];
        if (!subject_value.IsNull()) {
          subject = std::get<std::string>(subject_value);
        } else {
          subject = std::string();
        }
        text =
            std::get<std::string>(arguments[flutter::EncodableValue("text")]);
        LOG_INFO("subject[%s], text[%s]", subject.c_str(), text.c_str());
      }
    } catch (std::bad_variant_access const &ex) {
      result->Error("Invalid Arguments",
                    "An argument has to contain map<string, string>",
                    flutter::EncodableValue(ex.what()));
      return;
    }

    if (text.empty()) {
      result->Error("Invalid Arguments", "Non-empty text expected");
      return;
    }

    if (method_call.method_name().compare("shareFiles") == 0) {
      try {
        auto image_paths = std::get<flutter::EncodableList>(
            arguments[flutter::EncodableValue("paths")]);
        for (auto &&path : image_paths) {
          auto &&value = std::get<std::string>(path);
          path_array.push_back(value.c_str());
          LOG_INFO("image path [%s]", value.c_str());
        }

        auto mime_types = std::get<flutter::EncodableList>(
            arguments[flutter::EncodableValue("mimeTypes")]);
        mime_type = ReduceMimeTypes(mime_types);
        LOG_INFO("reduced mime type [%s]", mime_type.c_str());

      } catch (std::bad_variant_access const &ex) {
        result->Error("Invalid Arguments",
                      "An argument has to contain list<string>",
                      flutter::EncodableValue(ex.what()));
        return;
      }
    }

    // Create app control handler
    app_control_h handle;
    int ret = app_control_create(&handle);
    RET_IF_ERROR(ret);

    // Set operation
    ret = app_control_set_operation(handle, APP_CONTROL_OPERATION_SHARE_TEXT);
    RET_IF_ERROR(ret);

    // Set Uri (like mailto or sms)
    std::string url = "sms:";
    if (path_array.size() > 0 || subject.length() > 0) {
      url = "mailto:";
    }
    LOG_INFO("app_control_set_uri is set into [%s]", url.c_str());
    ret = app_control_set_uri(handle, url.c_str());
    RET_IF_ERROR(ret);

    // Set mime type & image paths
    if (path_array.size() > 0) {
      ret = app_control_set_mime(handle, mime_type.c_str());
      RET_IF_ERROR(ret);

      ret = app_control_add_extra_data_array(
          handle, APP_CONTROL_DATA_PATH, path_array.data(), path_array.size());
      RET_IF_ERROR(ret);
    }

    // Set subject (for email)
    if (subject.length() > 0) {
      ret = app_control_add_extra_data(handle, APP_CONTROL_DATA_SUBJECT,
                                       subject.c_str());
      RET_IF_ERROR(ret);
    }

    // Set text
    ret =
        app_control_add_extra_data(handle, APP_CONTROL_DATA_TEXT, text.c_str());
    RET_IF_ERROR(ret);

    ret = app_control_set_launch_mode(handle, APP_CONTROL_LAUNCH_MODE_GROUP);
    RET_IF_ERROR(ret);

    ret = app_control_send_launch_request(handle, nullptr, nullptr);
    RET_IF_ERROR(ret);

    if (handle) {
      app_control_destroy(handle);
    }

    result->Success();
  }

  std::string ReduceMimeTypes(flutter::EncodableList mimeTypes) {
    if (mimeTypes.size() > 1) {
      std::string reducedMimeType = std::get<std::string>(mimeTypes[0]);
      for (int i = 1; i < mimeTypes.size(); i++) {
        std::string mimeType = std::get<std::string>(mimeTypes[i]);
        if (reducedMimeType.compare(mimeType) != 0) {
          if (GetMimeTypeBase(mimeType).compare(
                  GetMimeTypeBase(reducedMimeType)) == 0) {
            reducedMimeType = GetMimeTypeBase(mimeType) + "/*";
          } else {
            reducedMimeType = "*/*";
            break;
          }
        }
      }
      return reducedMimeType;
    } else if (mimeTypes.size() == 1) {
      return std::get<std::string>(mimeTypes[0]);
    } else {
      return "*/*";
    }
  }

  std::string GetMimeTypeBase(std::string mimeType) {
    if (mimeType.empty() || mimeType.find('/') == std::string::npos) {
      return "*";
    }

    return mimeType.substr(0, mimeType.find('/'));
  }
};

void SharePlusTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  SharePlusTizenPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
