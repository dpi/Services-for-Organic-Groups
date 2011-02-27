<?php

/**
 * Get group content related to a group.
 * 
 * @param int $group_nid Node ID of a group node. Inherited.
 * @param string $type limit group content node types.
 * @param int $exclusive boolean the content must only have no other groups in its audience.
 */
function _group_resource_get_content_of_group($group_nid, $type = '*', $exclusive) {
  $group_content = array();
  $group_og = og_get_group('node', $group_nid);
  
  // query all nodes who have group_audience containing this group
  $group_content_query = new EntityFieldQuery;
  $group_content_query
  ->entityCondition('entity_type', 'node', '=')
  ->fieldCondition(OG_AUDIENCE_FIELD, 'gid', $group_og->gid, '=');
  
  if ($type != '*') {
    if (!og_is_group_content_type('node', $type)) {
      return services_error(t('Attempted to filter on group content types failed. Content type `@type` is not a valid group content type.', array('@type' => $type)), 400); // Bad request
    }
    $group_content_query->entityCondition('bundle', $type, '=');
  }
  
  $group_content_result = $group_content_query->execute();
  
  foreach ($group_content_result as $entity) {
    foreach ($entity as $entity_type => $node) {
      $content_node = node_load($node->nid);
      
      // Loop through other groups that own this content
      $content_group_nids = array();
      foreach ($content_node->{OG_AUDIENCE_FIELD} as $lang) {
        foreach ($lang as $group) {
          $group_entity = og_load($group['gid']);
          
          if ($group_entity->entity_type == 'node' && og_user_access($group['gid'], 'administer nodes')) {
            $content_group_nids[] = $group_entity->etid;
          }
        }
      }
      
      if ($exclusive && count($content_group_nids) > 1) {
        continue;
      }
      
      // silently fail if user does not have access
      if (node_access('view', $content_node)) {
        $group_content[$node->nid] = array(
        // TODO: Get all other groups/nids
          'nid' => $content_node->nid,
          'title' => $content_node->title,
          'type' => $content_node->type,
          'created' => $content_node->created,
          'changed' => $content_node->changed,
          'groups' => $content_group_nids,
        );
      }
    }
  }
  
  return $group_content;
}

/**
 * Get groups related to a group content.
 * 
 * @param int $content_nid Node ID of a group content node. Inherited
 * @param string $type limit groups of node type (optional)
 */
function _group_resource_get_group_of_content($content_nid, $type = '*') {
  $groups = array();
  
  $node = node_load($content_nid);
  $groups_entities = og_load_multiple(og_get_entity_groups('node', $node));
  
  if ($type != '*') {
    if (!og_is_group_type('node', $type)) {
      return services_error(t('Attempted to filter on group type failed. Content type `@type` is not a valid group type.', array('@type' => $type)), 406); // 400: Bad request
    }
  }
  
  foreach ($groups_entities as $group) {
    // Show all groups that this node belongs to.
    // No checking if user has access each group.
    if ($type != '*' && $group->entity_type == 'node') {
      $group_node = node_load($group->etid);
      if ($group_node->type != $type) { continue; }
    }
    
    $groups[$group->etid] = array(
      //'gid' => $group->gid,
      'nid' => $group->etid,
      'title' => $group->label,
      'created' => $group->created,
    );
  }
  
  return $groups;
}

function _group_resource_access($op, $args = array()) {
  $nid = $args[0];
  $node = node_load($nid);
  
  if (isset($node->nid) && $node->nid) {
    switch ($op) {
    case 'getContentRelatingToGroup':
      
      if (og_is_group_type('node', $node->type)) {
        // check if user has permission to view the group entity.
        $group_og = og_get_group('node', $node->nid);
        if (og_user_access($group_og->gid, 'administer nodes')) {
          return true;
        } else {
          return services_error('User does not have group `administer nodes` permission.', 406); // 403: Forbidden
        }
      } else {
        return services_error('This is not a group type.', 406); // 400 Bad request
      }
      
      break;
    case 'getGroupRelatingToContent':
      
      if (og_is_group_content_type('node', $node->type)) {
        if (node_access('view', $node)) {
          return true;
        }
      } else {
        return services_error('This is not a group content type.', 406); // 400 Bad request
      }
      
      break;
    }
  } else {
    return services_error('Node id: '. $nid .' could not be found.', 404);
  }
  
  return false;
}