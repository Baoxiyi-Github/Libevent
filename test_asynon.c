#include <event.h>
#include <stdio.h>

int main(void)
{
    const char **all_method = event_get_supported_methods();

    while(all_method && *all_method)
    {
        printf("%s\t", *all_method++);
    }

    printf("\n");

    struct event_base *base = event_base_new();
    if(base)
        printf("current method:\t%s\n", event_base_get_method(base));
    else
        printf("base == NULL\n");
    return 0;
}
