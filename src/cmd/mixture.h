#ifndef BYLINS_MIXTURE_H
#define BYLINS_MIXTURE_H

const short SCMD_ITEMS = 0;
const short SCMD_RUNES = 1;

class CharacterData;

void do_mixture(CharacterData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_MIXTURE_H
