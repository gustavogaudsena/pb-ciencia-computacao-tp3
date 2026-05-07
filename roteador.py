import ipaddress
import time

class NoBit:
    def __init__(self):
        self.bits = {0: None, 1: None}
        self.route_id = None
        self.eh_terminal = False
        self.versao = None
        self.skip_bits = 0
        self.path_value = ""
        
class TrieLPM:
    def __init__(self):
        self.raiz = NoBit()
                
    def _get_bit(self, ip_bits: int, pos: int, max_bits) -> int:
        if pos >= max_bits:
            return 0
        return (ip_bits >> (max_bits - 1 - pos)) & 1
    
    
    def inserir(self, prefix: str, route_id: int):
        rede = ipaddress.ip_network(prefix, strict=False)
        ip_int = int(rede.network_address)
        versao = rede.version
        prefixo = rede.prefixlen
        max_bits = rede.max_prefixlen

        no_atual = self.raiz
        for i in range(prefixo):
            bit = self._get_bit(ip_int, i, max_bits)
            if not no_atual.bits[bit]:
                no_atual.bits[bit] = NoBit()
            no_atual = no_atual.bits[bit]

        no_atual.eh_terminal = True
        no_atual.versao = versao
        no_atual.route_id = route_id
        
    def buscar(self, ip: str) -> str:
        ip_int = int(ipaddress.ip_address(ip))
        max_bits = ipaddress.ip_address(ip).max_prefixlen
        versao = ipaddress.ip_address(ip).version

        no_atual: NoBit = self.raiz
        last_route_id = no_atual.route_id if no_atual.eh_terminal and no_atual.versao == versao else None
        
        cursor = 0
        while cursor < max_bits:
            bit = self._get_bit(ip_int, cursor, max_bits)
            if not no_atual.bits[bit]:
                break

            no_atual = no_atual.bits[bit]
            cursor += 1

            if no_atual.skip_bits > 0:
                if cursor + no_atual.skip_bits > max_bits:
                    break

                mismatch = False
                for i in range(no_atual.skip_bits):
                    bit_skip = self._get_bit(ip_int, cursor + i, max_bits)
                    if bit_skip != int(no_atual.path_value[i]):
                        mismatch = True
                        break
                if mismatch:
                    break
                cursor += no_atual.skip_bits

            if no_atual.eh_terminal and no_atual.versao == versao:
                last_route_id = no_atual.route_id

        return last_route_id
    
    def compactar(self):
        self._compactar_no(self.raiz)

    def _compactar_no(self, no: NoBit):
        if no is None:
            return
        
        for bit in [0, 1]:
            if no.bits[bit]:
                self._compactar_no(no.bits[bit])
                
        if not no.eh_terminal and ((no.bits[0] and not no.bits[1]) or (not no.bits[0] and no.bits[1])):
             filho = no.bits[0] if no.bits[0] else no.bits[1]
             no.skip_bits += 1 + filho.skip_bits
             no.path_value = f"{no.path_value}{0 if no.bits[0] else 1}{filho.path_value}"
             no.bits[0] = filho.bits[0]
             no.bits[1] = filho.bits[1]
             
             no.eh_terminal = filho.eh_terminal
             no.route_id = filho.route_id
             no.versao = filho.versao
             self._compactar_no(no)
        


def contar_nos(no: NoBit) -> int:
    if no is None:
        return 0
    return 1 + contar_nos(no.bits[0]) + contar_nos(no.bits[1])


if __name__ == "__main__":
    arvore = TrieLPM()

    arvore.inserir("192.168.0.1/16", 1)
    arvore.inserir("192.168.1.0/24", 20)
    arvore.inserir("192.168.1.128/25", 30)
    arvore.inserir("10.0.0.0/8", 40)
    arvore.inserir("0.0.0.0/0", 50)
    arvore.inserir("2001:db8::/32", 100)
    arvore.inserir("2001:db8:a::/48", 200)

    ips_teste = [
        "192.168.0.50",
        "192.168.1.20",
        "192.168.1.150",
        "10.255.0.1",
        "8.8.8.8",
        "2001:db8:cafe::1",
        "2001:db8:a:b::1",
    ]
    n_iteracoes = 10000

    print(f"Nós antes da compactação: {contar_nos(arvore.raiz)}")
    t_inicio = time.perf_counter()
    for _ in range(n_iteracoes):
        for ip in ips_teste:
            arvore.buscar(ip)
    t_fim = time.perf_counter()
    print(f"Tempo sem compactação ({n_iteracoes * len(ips_teste)} buscas): {t_fim - t_inicio:.6f}s")

    arvore.compactar()

    print(f"Nós depois da compactação: {contar_nos(arvore.raiz)}")
    t_inicio = time.perf_counter()
    for _ in range(n_iteracoes):
        for ip in ips_teste:
            arvore.buscar(ip)
    t_fim = time.perf_counter()
    print(f"Tempo com compactação ({n_iteracoes * len(ips_teste)} buscas): {t_fim - t_inicio:.6f}s")

    for ip in ips_teste:
        print(f"Buscando IP {ip}: {arvore.buscar(ip)}")
