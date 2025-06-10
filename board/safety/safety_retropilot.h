// WIP: RP safety model
const CanMsg RETROPILOT_TX_MSGS[] = {{0x10E, 0, 6}, {0x12E, 0, 6}, {0x200, 0, 6}, 
                                     {0x20E, 0, 6}, {0x22E, 0, 5}, {0x300, 0, 6}, 
                                     {0x400, 0, 6}, {0x600, 0, 4},  // DSU bus 0
};

AddrCheckStruct retropilot_addr_checks[] = {
    {.msg = {{0x10F, 0, 6, .check_checksum = false, .expected_timestep = 12000U}, { 0 }, { 0 }}},
    {.msg = {{0x12F, 0, 6, .check_checksum = false, .expected_timestep = 20000U}, { 0 }, { 0 }}},
    {.msg = {{0x201, 0, 6, .check_checksum = false, .expected_timestep = 30000U}, { 0 }, { 0 }}},
    {.msg = {{0x20F, 0, 6, .check_checksum = false, .expected_timestep = 25000U}, { 0 }, { 0 }}},
    {.msg = {{0x22F, 0, 8, .check_checksum = false, .expected_timestep = 25000U}, { 0 }, { 0 }}},
    {.msg = {{0x301, 0, 6, .check_checksum = false, .expected_timestep = 12000U}, { 0 }, { 0 }}},
    {.msg = {{0x401, 0, 6, .check_checksum = false, .expected_timestep = 12000U}, { 0 }, { 0 }}},
    {.msg = {{0x601, 0, 6, .check_checksum = false, .expected_timestep = 12000U}, { 0 }}},
  };

#define RETROPILOT_ADDR_CHECKS_LEN (sizeof(retropilot_addr_checks) / sizeof(retropilot_addr_checks[0]))
addr_checks retropilot_rx_checks = {retropilot_addr_checks, RETROPILOT_ADDR_CHECKS_LEN};

// global actuation limit states

/*
// placeholder - implement CRC8_1D from ocelot
static uint8_t retropilot_compute_checksum(CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);
  int len = GET_LEN(to_push);
  uint8_t checksum = (uint8_t)(addr) + (uint8_t)((unsigned int)(addr) >> 8U) + (uint8_t)(len);
  for (int i = 0; i < (len - 1); i++) {
    checksum += (uint8_t)GET_BYTE(to_push, i);
  }
  return checksum;
}

static uint8_t retropilot_get_checksum(CANPacket_t *to_push) {
  return (uint8_t)(GET_BYTE(to_push, 0));
}
*/

static int retropilot_rx_hook(CANPacket_t *to_push) {
  bool valid = 1;
//   bool valid = addr_safety_check(to_push, &retropilot_rx_checks,
//                                  retropilot_get_checksum, retropilot_compute_checksum, NULL);

//   if (valid && (GET_BUS(to_push) == 0U)) {
//     int addr = GET_ADDR(to_push);
//     generic_rx_checks((addr == 0x2E4));
//   }
  if (valid && (GET_BUS(to_push) == 0U)) {
    int addr = GET_ADDR(to_push);
    if (addr == 0x201) {
        gas_interceptor_detected = 1;
        int gas_interceptor = TOYOTA_GET_INTERCEPTOR(to_push);
        gas_pressed = gas_interceptor > TOYOTA_GAS_INTERCEPTOR_THRSLD;
  
        // TODO: remove this, only left in for gas_interceptor_prev test
        gas_interceptor_prev = gas_interceptor;
      }
  }
  return valid;
}

static int retropilot_tx_hook(CANPacket_t *to_send) {

  int tx = 1;
  int addr = GET_ADDR(to_send);
  int bus = GET_BUS(to_send);

  if (!msg_allowed(to_send, RETROPILOT_TX_MSGS, sizeof(RETROPILOT_TX_MSGS)/sizeof(RETROPILOT_TX_MSGS[0]))) {
    tx = 0;
  }

  // Check if msg is sent on BUS 0
  if (bus == 0) {
    // GAS PEDAL: safety check
    if (addr == 0x200) {
      if (!controls_allowed) {
        if (GET_BYTE(to_send, 0) || GET_BYTE(to_send, 1)) {
          tx = 0;
        }
      }
    }
  }

  return tx;
}

static const addr_checks* retropilot_init(int16_t param) {
  UNUSED(param);
  controls_allowed = 0;
  relay_malfunction_reset();
  gas_interceptor_detected = 0;
  return &retropilot_rx_checks;
}

static int retropilot_fwd_hook(int bus_num, CANPacket_t *to_fwd) {
  // Forwarding is not implemented or required
  UNUSED(to_fwd);
  UNUSED(bus_num);
  int bus_fwd = -1;
  return bus_fwd;
}

const safety_hooks retropilot_hooks = {
  .init = retropilot_init,
  .rx = retropilot_rx_hook,
  .tx = retropilot_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = retropilot_fwd_hook,
};
