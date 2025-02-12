[@ocaml.warning "-27-26"];
open Bindings;
open Core;
open Ink;
open Common;
module Scaffold = Scaffold;

module Complete = {
  [@react.component]
  let make = (~configuration: Configuration.t) => {
    let config_str =
      Configuration.to_json(configuration)
      |> JavaScript.Json.stringify(~indent=2)
      |> React.string;

    React.useEffect0(() => {
      switch (configuration.bundler) {
      | Vite => Open.open_browser("localhost:5173")
      | Webpack => Open.open_browser("localhost:8080")
      | _ => ()
      };

      None;
    });

    <Box flexDirection=`column>
      <Text> {React.string("Your project configuration:")} </Text>
      <Spacer />
      <Text> config_str </Text>
    </Box>;
  };
};

[@react.component]
let make = (~name as initial_name) => {
  let (is_active, set_is_active) = React.useState(() => Some(true));
  let (env_check_result: option([ | `Pass | `Fail]), set_env_check_result) =
    React.useState(() => None);
  let (should_prompt_git, set_should_prompt_git) =
    React.useState(() => false);
  let (configuration, set_configuration) =
    React.useState(() => (None: option(Core.Configuration.t)));
  let (scaffold_result, set_scaffold_result) =
    React.useState(() => (None: option(result(unit, string))));

  let parsed_name_and_dir =
    React.useMemo1(
      () => Option.map(Core.Fs.parse_project_name_and_dir, initial_name),
      [|initial_name|],
    );

  let initial_name_is_valid =
    React.useMemo1(
      () =>
        parsed_name_and_dir
        |> Option.map(fst)
        |> Option.map(Core.Validation.Project_name.validate),
      [|parsed_name_and_dir|],
    );

  let on_env_check =
    React.useCallback0(result => {
      switch (result) {
      | `Pass(results) =>
        set_env_check_result(_ => Some(`Pass));
        let should_prompt_git =
          List.exists(
            result => {
              switch (result) {
              | `Fail(_) => true
              | `Pass(module Dep: Core.Dependency.S) =>
                Dep.name == "Git" ? true : false
              }
            },
            results,
          );

        set_should_prompt_git(_ => should_prompt_git);
        ();
      | `Fail(_) =>
        set_env_check_result(_ => Some(`Fail));
        set_is_active(_ => Some(false));
      }
    });

  let on_complete_wizard =
    React.useCallback0(configuration => {
      set_configuration(_ => Some(configuration))
    });

  let on_complete_scaffold =
    React.useCallback0(scaffold_result => {
      set_scaffold_result(_ => Some(scaffold_result))
    });

  React.useEffect1(
    () => {
      let _ =
        switch (scaffold_result) {
        | None => ()
        | Some(_) => set_is_active(_ => Some(false))
        };

      None;
    },
    [|scaffold_result|],
  );

  Ink.Hooks.use_input(
    (~input as _input, ~key as _key) => (),
    ~options={is_active: is_active},
  );

  let initial_configuration =
    React.useMemo1(
      () =>
        Core.Configuration.make_partial(
          ~name=?Option.map(fst, parsed_name_and_dir),
          ~directory=?Option.map(snd, parsed_name_and_dir),
          (),
        ),
      [|parsed_name_and_dir|],
    );

  <Box flexDirection=`column>
    {switch (initial_name_is_valid) {
     | Some(Error(`Msg(error))) =>
       <Ui.Badge color=`red> {React.string(error)} </Ui.Badge>
     | _ =>
       <>
         {Option.is_none(configuration) ? <Banner /> : React.null}
         <Env_check.Component onEnvCheck=on_env_check />
         {switch (env_check_result) {
          | Some(result) when result == `Pass =>
            <Wizard
              initial_configuration
              onComplete=on_complete_wizard
              should_prompt_git
            />
          | _ => React.null
          }}
         {switch (configuration) {
          | Some(configuration) =>
            <Scaffold configuration onComplete=on_complete_scaffold />
          | None => React.null
          }}
         {switch (scaffold_result, configuration) {
          | (Some(Ok(_)), Some(configuration)) => <Complete configuration />
          | (None, _) => React.null
          | _ => React.null
          }}
       </>
     }}
  </Box>;
};
