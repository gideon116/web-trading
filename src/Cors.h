#pragma once
#include <crow.h>

struct Cors {
    struct context {};

    void before_handle(crow::request& req,
                       crow::response& res,
                       context&)
    {
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers","content-type");

        if (req.method == "OPTIONS"_method) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(crow::request&,
                      crow::response& res,
                      context&)
    {
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers","content-type");
    }
};
