#include <event2/event.h>
#include <stdio.h>

int main(void)
{
    struct event_config *cfg;
    struct event_base *base;
    int i;

    for(i = 0; i < 2; i++)
    {
        cfg = event_config_new();
        event_config_avoid_method(cfg, "select");
        /*
         *if(i == 0)
         *  event_config_require_features(cfg, EV_FEATURE_ET);
         */

        base = event_base_new_with_config(cfg);
        event_config_free(cfg);
        if(base)
          break;
    }


    const char **all_method = event_get_supported_methods();
    
    while(all_method && *all_method)
    {
        printf("%s \t", *all_method++);
    }

    printf("\n");
    return 0;
}
