#pragma once
#include "IDGenerator.h"
#include "Order.h"
#include "OrderMatcher.h"

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/OrderCancelRequest.h>
#include <quickfix/fix42/MarketDataRequest.h>
#include <quickfix/fix42/ExecutionReport.h>

#include <functional>
#include <mutex>
#include <queue>

class OrderMatchApp : public FIX::Application,
                      public FIX::MessageCracker {
public:
  explicit OrderMatchApp(
        std::function<void(const FIX42::ExecutionReport&)> execCb)
      : execCb_(std::move(execCb)) {}

  void onCreate (const FIX::SessionID&) override {}
  void onLogon  (const FIX::SessionID& s) override { session_ = s; }
  void onLogout (const FIX::SessionID&) override {}
  void toAdmin  (FIX::Message&, const FIX::SessionID&) override {}
  void fromAdmin(const FIX::Message&, const FIX::SessionID&) 
  EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
  void toApp    (FIX::Message&, const FIX::SessionID&)
                     EXCEPT(FIX::DoNotSend) override {}

  void fromApp(const FIX::Message& m,const FIX::SessionID& s)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
    crack(m, s);            // dispatch to onMessage()
  }

  /* ------------ Message handlers ------------------------ */
  void onMessage(const FIX42::NewOrderSingle&,
                 const FIX::SessionID&) override;
  void onMessage(const FIX42::OrderCancelRequest&,
                 const FIX::SessionID&) override;
  void onMessage(const FIX42::MarketDataRequest&,
                 const FIX::SessionID&) override; 

private:
  void processOrder (const Order&);
  void processCancel(const std::string& id,
                     const std::string& symbol,
                     Order::Side side);

  void updateOrder (const Order&, char status);
  void rejectOrder (const Order&);
  void acceptOrder (const Order& o){ updateOrder(o, FIX::OrdStatus_NEW); }
  void fillOrder   (const Order& o){ updateOrder(o,
                            o.isFilled()?FIX::OrdStatus_FILLED
                                        :FIX::OrdStatus_PARTIALLY_FILLED); }
  void cancelOrder (const Order& o){ updateOrder(o, FIX::OrdStatus_CANCELED); }

  void rejectOrder(const FIX::SenderCompID&,
                   const FIX::TargetCompID&,
                   const FIX::ClOrdID&,
                   const FIX::Symbol&,
                   const FIX::Side&,
                   const std::string& why);

  static Order::Side convert(const FIX::Side&);
  static Order::Type convert(const FIX::OrdType&);
  static FIX::Side   convert(Order::Side);
  static FIX::OrdType convert(Order::Type);

  FIX::SessionID   session_;
  OrderMatcher     matcher_;
  IDGenerator      idGen_;
  std::mutex       m_;
  std::function<void(const FIX42::ExecutionReport&)> execCb_;
};
