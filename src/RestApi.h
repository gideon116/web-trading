#pragma once
#include <crow.h>  
class TradeClientApp;

namespace RestApi {
  template<typename App>
  void mount(App& app,
             TradeClientApp& trade);
}
