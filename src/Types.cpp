#include  "Types.hpp"

std::string toString(Language l){
    std::string str;
    switch(l)
    {
        case Language::English:
            str = "English";
            break;
        case Language::Chinese:
            str = "Chinese";
            break;
        default :
            break;
    }
    return str;
}