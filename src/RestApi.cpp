#include "Cors.h"  
#include "RestApi.h"
#include "TradeClientApp.h"
#include <crow.h> 
#include <iostream>
#include <ctime>
#include <quickfix/Session.h>

namespace RestApi {
  template<typename App>
  void mount(App& app, TradeClientApp& trade) {
      CROW_ROUTE(app, "/api/order").methods("POST"_method)
      ([&](const crow::request& req){
        auto j = crow::json::load(req.body);
        
        if (!j) {
          std::cerr << "[ERROR] /api/order bad JSON: " << req.body << "\n";
          return crow::response(400, R"({"error":"bad_json"})");
        }
    
    std::cout << "[REST] /api/order: " << j << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
    std::string sender = j["client"].s();
    bool isBuy = j["side"].s() == "BUY";
    std::string id = std::to_string(std::time(nullptr));

    trade.sendNewOrder(
      sender, id, j["symbol"].s(), j["qty"].i(),
      j.has("price") ? j["price"].d() : 0.0,
      !j.has("price"),
      FIX::Side(isBuy ? FIX::Side_BUY : FIX::Side_SELL));
      
    crow::json::wvalue w; w["clientOrderId"]=id;
    return crow::response(201, w.dump());
  });
}

template void RestApi::mount<crow::App<Cors>>(
    crow::App<Cors>&,
    TradeClientApp&
  );
}
