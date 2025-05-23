#include "TradeClientApp.h"
#include <quickfix/Session.h>
#include <iostream>

void TradeClientApp::onLogon(const FIX::SessionID& s) {
  std::lock_guard<std::mutex> g(m_);
  sessionMap_[s.getSenderCompID().getValue()] = s;
  std::cout << "[FIX] Logged on: " << s << '\n';
}

void TradeClientApp::sendNewOrder(const std::string& sender,
                                  const std::string& clID,
                                  const std::string& sym,
                                  int qty, double px,
                                  bool isMarket,
                                  FIX::Side side) {
  std::lock_guard<std::mutex> g(m_);
  auto it = sessionMap_.find(sender);
  if (it == sessionMap_.end())
    throw FIX::SessionNotFound();

  FIX42::NewOrderSingle msg(
      FIX::ClOrdID(clID),
      FIX::HandlInst('1'),
      FIX::Symbol(sym),
      side,
      FIX::TransactTime(),
      FIX::OrdType(isMarket ? FIX::OrdType_MARKET
                            : FIX::OrdType_LIMIT));

  msg.set(FIX::OrderQty(qty));
  if (!isMarket) msg.set(FIX::Price(px));

  FIX::Session::sendToTarget(msg, it->second);
}
