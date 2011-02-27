<?php


function _group_user_resource_getGroupsOfUser($uid, $role = NULL) {
  $group_nodes = array();
  
  $account = user_load($uid);
  $groups = og_get_entity_groups('user', $account);
  $groups = og_load_multiple($groups);
  
  foreach ($groups as $gid => $group) {
    $entities = entity_load($group->entity_type, array($group->etid));
    foreach ($entities as $entity) {
      $custom_entity = array(
        'title' => $entity->title,
        'nid' => $entity->nid,
        'roles' => og_get_user_roles_name(og_get_user_roles($gid, $uid)),
      );
      
      $group_nodes[] = $custom_entity;
    }
  }
  
  return $group_nodes;
}

function _group_user_resource_access($op, $args = array()) {
  $account = user_load($args[0]);
  if(!$account->uid) {
    return services_error('User does not exist.' . $args[0], 400); // Bad request
  }
  return user_view_access($account);
}