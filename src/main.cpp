#include <crow.h>                 // Crow single-header
#include "Cors.h"  
#include "TradeClientApp.h"
#include "OrderMatchApp.h"
#include "ExecutorApp.h"
#include "RestApi.h"

#include <quickfix/SocketInitiator.h>
#include <quickfix/SocketAcceptor.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/SessionSettings.h>
#include <algorithm>
#include <memory>
#include <vector>

struct Hub {
  std::vector<crow::websocket::connection*> clients;

  void broadcast(const FIX42::ExecutionReport& rpt) {

    const std::string clId = rpt.getField(FIX::FIELD::ClOrdID);
    const std::string ordStatus = rpt.getField(FIX::FIELD::OrdStatus);

    const std::string json = "{\"id\":\"" + clId + "\",\"status\":\"" + ordStatus + "\"}";
    for (auto* c : clients) c->send_text(json);}

} hub;


template<class APP, class PPT>
std::unique_ptr<PPT> startFix(const std::string& cfgFile, APP& app) {
  FIX::SessionSettings s(cfgFile);
  FIX::FileStoreFactory store(s);
  FIX::ScreenLogFactory log  (s);
  auto ptr = std::make_unique<PPT>(app, store, s, log);
  ptr->start();
  return ptr;
}

int main() {

  TradeClientApp trade([&](auto& rpt){ hub.broadcast(rpt); });
  
  OrderMatchApp  match([&](auto& rpt){ hub.broadcast(rpt); });
  ExecutorApp    exec ([&](auto& rpt){ hub.broadcast(rpt); });



  auto omAcc  = startFix<OrderMatchApp, FIX::SocketAcceptor>("../cfg/ordermatch.cfg",  match);
  // auto exAcc  = startFix<ExecutorApp,    FIX::SocketAcceptor>("../cfg/executor.cfg",    exec );
  auto client = startFix<TradeClientApp, FIX::SocketInitiator>("../cfg/tradeclient.cfg", trade);


  
  crow::App<Cors> app;
  RestApi::mount(app, trade);

  CROW_WEBSOCKET_ROUTE(app, "/ws").onopen(
    [&](crow::websocket::connection& c){hub.clients.push_back(&c);}).onclose(
      [&](crow::websocket::connection& c,
               const std::string&,
               unsigned short){hub.clients.erase(std::remove(hub.clients.begin(),
                hub.clients.end(), &c),
                hub.clients.end());
              });

  app.port(8080).multithreaded().run();
}
